package watchstudio;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

import java.awt.BorderLayout;
import java.awt.Desktop;
import java.awt.FlowLayout;
import java.awt.GraphicsEnvironment;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.BindException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.WindowConstants;

/**
 * Watch Studio as a Java app.
 *
 * The app itself is app/index.html, packed inside this jar. This class serves it on
 * http://localhost (only this computer can reach it) and opens it in Chrome or Edge in
 * app mode, which gives it its own window. A browser is still what draws the page,
 * because only Chromium browsers can talk to the watch over USB (Web Serial) and
 * Bluetooth (Web Bluetooth); localhost counts as a secure address, so both work.
 *
 * Nothing is sent anywhere: the server only answers requests from this computer.
 * A config.js next to the jar (the same file the installers write) adds the
 * personal touches.
 *
 * Usage: java -jar WatchStudio.jar [--port N] [--no-browser] [--no-window]
 */
public final class WatchStudio {
    /** Fixed, so the browser keeps the app's saved state and USB permissions between runs. */
    static final int DEFAULT_PORT = 47821;
    static final String[] FILES = {"index.html", "icon.svg", "icon.png", "manifest.webmanifest", "sw.js", "oled.js", "oled-assets.js"};
    static final Map<String, String> TYPES = new HashMap<>();

    static {
        TYPES.put("html", "text/html; charset=utf-8");
        TYPES.put("js", "text/javascript; charset=utf-8");
        TYPES.put("svg", "image/svg+xml");
        TYPES.put("png", "image/png");
        TYPES.put("webmanifest", "application/manifest+json");
    }

    private WatchStudio() {
    }

    public static void main(String[] args) throws IOException {
        int port = DEFAULT_PORT;
        boolean openBrowser = true;
        boolean window = !GraphicsEnvironment.isHeadless();
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "--port": port = Integer.parseInt(args[++i]); break;
                case "--no-browser": openBrowser = false; break;
                case "--no-window": window = false; break;
                default:
                    System.err.println("usage: java -jar WatchStudio.jar [--port N] [--no-browser] [--no-window]");
                    System.exit(2);
            }
        }

        Map<String, byte[]> files = new HashMap<>();
        for (String name : FILES) {
            byte[] data = resource("/app/" + name);
            if (data != null) {
                files.put(name, data);
            }
        }
        if (!files.containsKey("index.html")) {
            System.err.println("index.html is missing from the jar: build it with java-app/build.py");
            System.exit(1);
        }

        HttpServer server = start(port, files);
        int actual = server.getAddress().getPort();
        if (actual != port) {
            System.out.println("Port " + port + " was busy; using " + actual
                    + " (the browser keeps settings per port, so they may look reset)");
        }
        String url = "http://localhost:" + actual + "/";
        System.out.println("Watch Studio is at " + url);

        if (openBrowser) {
            open(url);
        }
        if (window) {
            final String u = url;
            SwingUtilities.invokeLater(() -> showWindow(u, server));
        } else {
            System.out.println("Press Ctrl+C to stop.");
        }
    }

    /** Binds to the loopback address only; tries the next few ports if one is taken. */
    static HttpServer start(int port, Map<String, byte[]> files) throws IOException {
        IOException last = null;
        for (int p = port; p < port + 10; p++) {
            try {
                HttpServer server = HttpServer.create(new InetSocketAddress(InetAddress.getLoopbackAddress(), p), 0);
                server.createContext("/", ex -> serve(ex, files));
                server.start();
                return server;
            } catch (BindException e) {
                last = e;
            }
        }
        throw last;
    }

    static void serve(HttpExchange ex, Map<String, byte[]> files) throws IOException {
        try {
            String method = ex.getRequestMethod();
            if (!method.equals("GET") && !method.equals("HEAD")) {
                send(ex, 405, "text/plain", "method not allowed".getBytes(StandardCharsets.UTF_8));
                return;
            }
            String path = ex.getRequestURI().getPath();
            String name = path.equals("/") ? "index.html" : path.substring(1);
            byte[] body;
            if (name.equals("config.js")) {
                body = configJs();
            } else {
                body = files.get(name);
            }
            if (body == null) {
                send(ex, 404, "text/plain", "not found".getBytes(StandardCharsets.UTF_8));
                return;
            }
            String ext = name.substring(name.lastIndexOf('.') + 1).toLowerCase(Locale.ROOT);
            send(ex, 200, TYPES.getOrDefault(ext, "application/octet-stream"), body);
        } finally {
            ex.close();
        }
    }

    static void send(HttpExchange ex, int code, String type, byte[] body) throws IOException {
        ex.getResponseHeaders().set("Content-Type", type);
        ex.getResponseHeaders().set("Cache-Control", "no-cache");
        ex.getResponseHeaders().set("X-Content-Type-Options", "nosniff");
        boolean head = ex.getRequestMethod().equals("HEAD");
        ex.sendResponseHeaders(code, head ? -1 : body.length);
        if (!head) {
            try (OutputStream out = ex.getResponseBody()) {
                out.write(body);
            }
        }
    }

    /** config.js beside the jar, if there is one: the personal touches (name, message, colour). */
    static byte[] configJs() {
        try {
            File jar = new File(WatchStudio.class.getProtectionDomain().getCodeSource().getLocation().toURI());
            File cfg = new File(jar.getParentFile(), "config.js");
            if (cfg.isFile()) {
                return Files.readAllBytes(cfg.toPath());
            }
        } catch (Exception ignored) {
            // no config: the app uses its defaults
        }
        return "// No config.js next to WatchStudio.jar: default look.\n".getBytes(StandardCharsets.UTF_8);
    }

    static byte[] resource(String name) throws IOException {
        try (InputStream in = WatchStudio.class.getResourceAsStream(name)) {
            if (in == null) {
                return null;
            }
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            for (int n; (n = in.read(buf)) > 0; ) {
                out.write(buf, 0, n);
            }
            return out.toByteArray();
        }
    }

    // ------------------------------------------------------------------ browser

    /** Chrome or Edge in app mode if one is installed, otherwise the default browser. */
    static void open(String url) {
        for (String exe : chromiumCandidates()) {
            try {
                if (exe.contains(File.separator) && !new File(exe).isFile()) {
                    continue;
                }
                new ProcessBuilder(exe, "--app=" + url, "--new-window").start();
                return;
            } catch (IOException ignored) {
                // try the next one
            }
        }
        System.out.println("Chrome or Edge not found. Opening the default browser: USB and Bluetooth"
                + " need Chrome or Edge, so install one of them to connect the watch.");
        try {
            if (Desktop.isDesktopSupported() && Desktop.getDesktop().isSupported(Desktop.Action.BROWSE)) {
                Desktop.getDesktop().browse(new URI(url));
            }
        } catch (Exception e) {
            System.out.println("Open " + url + " in Chrome or Edge.");
        }
    }

    static List<String> chromiumCandidates() {
        String os = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        List<String> out = new ArrayList<>();
        if (os.contains("win")) {
            for (String base : new String[]{System.getenv("ProgramFiles(x86)"), System.getenv("ProgramFiles"),
                    System.getenv("LOCALAPPDATA")}) {
                if (base == null) {
                    continue;
                }
                out.add(base + "\\Microsoft\\Edge\\Application\\msedge.exe");
                out.add(base + "\\Google\\Chrome\\Application\\chrome.exe");
            }
        } else if (os.contains("mac")) {
            String home = System.getProperty("user.home");
            for (String root : new String[]{"", home}) {
                out.add(root + "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome");
                out.add(root + "/Applications/Microsoft Edge.app/Contents/MacOS/Microsoft Edge");
                out.add(root + "/Applications/Chromium.app/Contents/MacOS/Chromium");
            }
        } else {
            out.addAll(Arrays.asList("google-chrome", "google-chrome-stable", "chromium", "chromium-browser",
                    "microsoft-edge", "microsoft-edge-stable"));
        }
        return out;
    }

    // ------------------------------------------------------------------- window

    /** A small window: closing it stops Watch Studio. */
    static void showWindow(String url, HttpServer server) {
        JFrame f = new JFrame("Watch Studio");
        JLabel text = new JLabel("<html><b>Watch Studio is running.</b><br>"
                + "It opened in Chrome or Edge. Close this window to stop it.</html>");
        text.setBorder(BorderFactory.createEmptyBorder(14, 16, 8, 16));
        JButton again = new JButton("Open Watch Studio");
        again.addActionListener(e -> open(url));
        JButton quit = new JButton("Stop");
        quit.addActionListener(e -> {
            server.stop(0);
            System.exit(0);
        });
        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        buttons.add(again);
        buttons.add(quit);
        f.setLayout(new BorderLayout());
        f.add(text, BorderLayout.CENTER);
        f.add(buttons, BorderLayout.SOUTH);
        f.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
        f.pack();
        f.setLocationRelativeTo(null);
        f.setVisible(true);
    }
}
