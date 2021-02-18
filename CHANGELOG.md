# Changelog

#### Table of Contents
- [Unreleased](#unreleased)
- [1.1.3](#113---2021-02-18)
- [1.1.2](#112---2020-11-03)
- [1.1.1](#111---2020-10-27)
- [1.1.0](#110---2020-10-26)
- [1.0.2](#102---2020-10-04)
- [1.0.1](#101---2020-10-03)
- [1.0.0](#100---2020-10-02)

___

## [Unreleased][]

## [1.1.3][] - 2021-02-18

### Fixed
- Fixed broken import-by-URL handling ([#37](https://github.com/TheQwertiest/foo_spotify/issues/37)).
- Fixed some issues with keyboard navigation in component menu in Preferences.

## [1.1.2][] - 2020-11-03

### Changed
- Better Web API handling in attempt to avoid 429 errors:
  - Pre-emptive batch artist update on album import ([#36](https://github.com/TheQwertiest/foo_spotify/issues/36)).

### Fixed
- Unreasonably high CPU usage in some scenarios ([#35](https://github.com/TheQwertiest/foo_spotify/issues/35)).
- Rps limiter is not working ([#34](https://github.com/TheQwertiest/foo_spotify/issues/34)).
- Extra silence at the end of track ([#33](https://github.com/TheQwertiest/foo_spotify/issues/33)).

## [1.1.1][] - 2020-10-27

### Changed
- Better Web API handling in attempt to avoid 429 errors:
  - Pre-emptive batch artist update on playlist import ([#32](https://github.com/TheQwertiest/foo_spotify/issues/32)).

## [1.1.0][] - 2020-10-26

### Added
- *Play* the artist: adds top tracks for the artist ([#15](https://github.com/TheQwertiest/foo_spotify/issues/15)).
- `%codec%` metadata field ([#14](https://github.com/TheQwertiest/foo_spotify/issues/14)).
- More options in `Preferences`:
  - Cache size configuration ([#23](https://github.com/TheQwertiest/foo_spotify/issues/23)).
  - Spotify `private mode` option ([#13](https://github.com/TheQwertiest/foo_spotify/issues/13)).
  - Spotify normalization option ([#12](https://github.com/TheQwertiest/foo_spotify/issues/12)).

### Changed
- Better Web API handling in attempt to avoid 429 errors:
  - RPS limit ([#27](https://github.com/TheQwertiest/foo_spotify/issues/27)).
  - `retry-after` response header handling ([#11](https://github.com/TheQwertiest/foo_spotify/issues/11)).
  - Pre-emptive batch track update on playlist change ([#30](https://github.com/TheQwertiest/foo_spotify/issues/30)).
  - Pre-emptive batch artist update on playlist change ([#31](https://github.com/TheQwertiest/foo_spotify/issues/31)).
- Improved error message when track is not available in user's country ([#8](https://github.com/TheQwertiest/foo_spotify/issues/8)).

### Fixed
- Can't relogin without closing Preferences dialog ([#24](https://github.com/TheQwertiest/foo_spotify/issues/24)).
- Dynamic info change is signaled too frequently, despite data being the same ([#22](https://github.com/TheQwertiest/foo_spotify/issues/22)).
- Can't play tracks when buffering is enabled in fb2k ([#21](https://github.com/TheQwertiest/foo_spotify/issues/21)).
- Crash: Illegal operation ([#19](https://github.com/TheQwertiest/foo_spotify/issues/19)).
- Multi-value tags are not displayed correctly ([#17](https://github.com/TheQwertiest/foo_spotify/issues/17)).
- Can't import playlists with local tracks ([#16](https://github.com/TheQwertiest/foo_spotify/issues/16), [#18](https://github.com/TheQwertiest/foo_spotify/issues/18)).

## [1.0.2][] - 2020-10-04

### Added
- Proxy support ([#6](https://github.com/TheQwertiest/foo_spotify/issues/6)).
- Adjust bit-rate via `Preferences` ([#5](https://github.com/TheQwertiest/foo_spotify/issues/5)).
- Bit-rate metadata for playing track ([#4](https://github.com/TheQwertiest/foo_spotify/issues/4)).

### Fixed
- Fails to open track that has null `preview_url` ([#3](https://github.com/TheQwertiest/foo_spotify/issues/3)).

## [1.0.1][] - 2020-10-03

### Fixed
- LibSpotify login doesn't always work ([#1](https://github.com/TheQwertiest/foo_spotify/issues/1)).

## [1.0.0][] - 2020-10-02
Initial release.

[unreleased]: https://github.com/TheQwertiest/foo_spotify/compare/v1.1.3...HEAD
[1.1.3]: https://github.com/TheQwertiest/foo_spotify/compare/v1.1.2...v1.1.3
[1.1.2]: https://github.com/TheQwertiest/foo_spotify/compare/v1.1.1...v1.1.2
[1.1.1]: https://github.com/TheQwertiest/foo_spotify/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/TheQwertiest/foo_spotify/compare/v1.0.2...v1.1.0
[1.0.2]: https://github.com/TheQwertiest/foo_spotify/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/TheQwertiest/foo_spotify/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/TheQwertiest/foo_spotify/commits/master
