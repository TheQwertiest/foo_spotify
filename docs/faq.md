---
title: Frequesntly Asked Questions
nav_order: 3
---

# Installation
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

#### How to actually play music from Spotify?

Pass the music URL to foobar2000 via main menu `File`>`Add location...` (`CTRL+U` by default).  
URL can be accessed from any track/album/playlist in Spotify by clicking on `...`>`Copy Song Link`/`Copy Album Link`/`Copy Playlist Link`.
<br><br>

#### Why does it fail with `Unable to open item for playback (Object not found)` when I pass album or playlist via URI?

A foobar2000 limitation: it's not possible (or rather REALLY hard) to use an arbitrary string to load multiple tracks.  
It works for the track, because, well, it's a single track =)
<br><br>

#### Why I can't play and/or see some tracks?

Spotify interface limitation: podcasts and radios are not supported.
<br><br>

#### Why I can't control playback from Spotify website?
#### Why I can't set foobar2000 as output device from Spotify website?

Spotify interface limitation: these features are part of the Spotify Connect interface, which is not publicly accessible.
<br><br>

#### Why two separate authentications are required?

The component uses two separate interfaces for Spotify integration: 
- `LibSpotify` for music playback.
- `WebAPI` for everything else. 

Each one uses a separate authentication scheme.