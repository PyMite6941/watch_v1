# Running Watch Studio: web site or Java app

Watch Studio is one page, [`app/index.html`](../app/index.html). Besides the Windows and macOS installers ([`installer/`](../installer/README.md)), there are two more ways to run it. All of them run the same page, so they look and work the same.

| Way | Good for | Needs |
|---|---|---|
| Installer | Giving it as a gift: shortcuts, personal message | Windows or macOS, with Edge or Chrome |
| **Web site (Vercel)** | Anyone with the link, any computer, nothing installed. Always the newest version | Chrome or Edge (any OS, also Android) |
| **Java app** | School computers where installers are blocked but Java is allowed; Linux | Java 11 or newer, plus Chrome or Edge |

Whichever way it runs, the page talks to the watch over USB (Web Serial) or Bluetooth (Web Bluetooth). Only Chromium browsers (Chrome, Edge, Opera, Brave) have these; Safari and Firefox do not. Browsers allow them only on secure addresses: `https://…` (the web site) or `http://localhost` (the Java app).

## Web site on Vercel

The repository is ready to deploy: [`vercel.json`](../vercel.json) at the top tells Vercel to publish the `app/` folder as it is (no build step) and sets the headers:

- **No caching** for the page itself, the service worker and the manifest. This means a new version shows up straight away.
- **USB and Bluetooth** allowed for the site (`Permissions-Policy`).

To put it online:

1. Sign in at [vercel.com](https://vercel.com) with GitHub. The free Hobby plan is enough: it is a few small files.
2. **Add New → Project**, choose the `watch_v1` repository, and leave every setting as it is (*Framework: Other*; Vercel reads `vercel.json`). Click **Deploy**.
3. You get an address like `https://watch-v1.vercel.app`. Every push to `main` updates it. Pull requests get their own preview address.

Or from a terminal, in the repository folder: `npx vercel` (preview), then `npx vercel --prod`.

Once it has been opened, the page keeps working without internet (its service worker caches it). In Chrome or Edge, the *Install* icon in the address bar adds it to the Start menu or Dock like an app.

**What the web version does not have:** the personal touches (name, message, colour) that the installers write. It always uses the default look. Each browser keeps its own settings and permissions for the site.

**Privacy:** the site is only files. The page talks to the watch inside the browser; nothing about the watch or its owner is sent to Vercel. Vercel keeps normal request logs (address, time) like any web host.

## Java app

[`java-app/`](../java-app) packs the page into a single file, `WatchStudio.jar` (about 35 kB, no other libraries). Running it:

1. starts a tiny web server that only this computer can reach (`http://localhost:47821`, or the next free port);
2. opens that address in Chrome or Edge as an app window (no address bar), or the default browser if neither is found;
3. shows a small "Watch Studio is running" window. Closing it, or **Stop**, quits.

Build it (needs a JDK 11 or newer):

```bash
python java-app/build.py          # -> dist/WatchStudio.jar
java -jar dist/WatchStudio.jar    # or double-click the jar
```

Options: `--port N` uses a different port. `--no-browser` does not open a browser. `--no-window` runs without the small window, for servers or headless use (stop it with Ctrl+C).

The port stays the same between runs on purpose. The browser keeps the app's settings and its USB permission per address, so a changing port would make them look reset each time.

For personal touches, put a `config.js` next to the jar. Use the same file the installers write, for example `window.STUDIO_CONFIG = { forName: "Sam", message: "Happy teaching!", accent: "blue" };`.

Run `build.py` again after changing anything in `app/`; the jar carries its own copy.
