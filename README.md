Muon is a framework that leverages the full power of [Chromium](https://www.chromium.org/Home) including extensions
support, and allows you to build browsers and browser like applications with HTML, CSS, and JavaScript.  Node is added
into the browser process only for security reasons.

It may be a better fit than [Electron](https://github.com/electron/electron) for your application, if your application
needs to leverage the full support of Chromium, needs tighter security, or needs support for things like autofill and
extensions.

Some of Muons goals include:
- use the Chromium source directly (eliminating electron's copy of `chrome_src`) with minor patches
  - make integrating chrome components less painful
  - faster and more streamlined end-to-end build process (see [browser-laptop-bootstrap](https://github.com/brave/browser-laptop-bootstrap))
- add support for Chrome extensions
- add security focused features for the renderer:
  - remove node completely (from the renderer process)
  - full sandbox
  - scriptable window.opener support

Muon is a fork of the [Electron](https://github.com/electron/electron/) framework
which is currently used in the [Brave](https://brave.com) web browser.

Follow [@brave](https://twitter.com/brave) on Twitter for important
announcements.

## Downloads

Prebuilt binaries and debug symbols of Muon for Linux, Windows and macOS can
be found on the [releases](https://github.com/brave/muon/releases) page.

## Documentation

Guides and the API reference are located in the
[docs](https://github.com/brave/muon/tree/master/docs) directory.

You can also [see our wiki](https://github.com/brave/browser-laptop-bootstrap/wiki) for tips on building Muon.

## Quick Start

Clone and run the [`muon-quick`](https://github.com/jonathansampson/muon-quick)
repository to see a minimal Muon app in action.

## Community

You can ask questions and interact with the community in the following
locations:
- [The Brave Community](https://community.brave.com/)
- [`community`](https://bravesoftware.slack.com) channel on Brave Software's Slack
- [`brave`](https://electronhq.slack.com) channel on Electron HQ's Slack
