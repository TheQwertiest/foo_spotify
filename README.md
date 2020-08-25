# Spotify Integration
[![version][version_badge]][changelog] [![Build status][appveyor_badge]](https://ci.appveyor.com/project/TheQwertiest/foo-spotify/branch/master) [![CodeFactor][codefactor_badge]](https://www.codefactor.io/repository/github/theqwertiest/foo_spotify/overview/master) [![Codacy Badge][codacy_badge]](https://app.codacy.com/app/qwertiest/foo_spotify?utm_source=github.com&utm_medium=referral&utm_content=TheQwertiest/foo_spotify&utm_campaign=Badge_Grade_Dashboard) 

This is a component for the [foobar2000](https://www.foobar2000.org) audio player, which allows to play tracks from Spotify.

Features:
- Import of albums and playlists.
- Album art fetching.
- [foo_acfu](https://acfu.3dyd.com) integration.

## Prerequisites

- Windows 7 or later.
- foobar2000 v1.3.17 or later.
- Spotify Premium account.

## Installation

1. Remove `foo_spotify_input` component if present.
1. Download the [latest release](https://github.com/TheQwertiest/foo_spotify/releases/latest) (you only need `foo_spotify.fb2k-component`).
1. Install the component using the [following instructions](http://wiki.hydrogenaud.io/index.php?title=Foobar2000:How_to_install_a_component).
1. Authorize the component via `Preferences`>`Spotify Integration`.

## FAQ

#### Why two separate authentications are required?

The component uses two separate interfaces for Spotify integration: 
- `LibSpotify` for music playback.
- `WebAPI` for everything else. 

Each one uses a separate authentication scheme.

#### Why I can't play and/or see some tracks?

Spotify interface limitation: podcasts and radios are not supported.

#### Why I can't control playback from Spotify website?
#### Why I can't set foobar2000 as output device from Spotify website?

Spotify interface limitation: these features are part of the Spotify Connect interface, which is not publicly accessible.


## Links
[Changelog][changelog]  
[Current tasks and plans][todo]  
[Nightly build](https://ci.appveyor.com/api/projects/theqwertiest/foo-spotify/artifacts/_result%2FWin32_Release%2Ffoo_spotify.fb2k-component?branch=master&job=Configuration%3A%20Release)

## Credits

- [Respective authors](THIRD_PARTY_NOTICES.md) of the code being used in this project.

[changelog]: CHANGELOG.md
[todo]: https://github.com/TheQwertiest/foo_spider_monkey_panel/projects/1
[version_badge]: https://img.shields.io/github/release/theqwertiest/foo_spotify.svg
[appveyor_badge]: https://ci.appveyor.com/api/projects/status/t5bhoxmfgavhq81m/branch/master?svg=true
[codacy_badge]: https://api.codacy.com/project/badge/Grade/319298ca5bd64a739d1e70e3e27d59ab
[codefactor_badge]: https://www.codefactor.io/repository/github/theqwertiest/foo_spotify/badge/master
