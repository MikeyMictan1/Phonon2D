<h1 align="center">Phonon2D - A high-performance C++ GDExtension for real-time acoustic ray-tracing in 2D Environments.</h1>
What if audio had physics? Sounds will bounce around the scene, get absorbed or amplified by certain materials, and interact with other objects! Built for 2D for the Godot Engine. Named after 'phonons', a discrete packet of vibrational energy (the fundamental carrier of sound).

## Tech Stack
**Languages** : C++, GDScript <br>
**Engine** : Godot <br>
**Dimension** : 2D (for now) <br>

## Library Structure
`PhononPacket` : Individual sound packet objects that bounce around a scene, simulating sound.

`PhononEmitter2D` : Inherits from `AudioStreamPlayer2D`, emitting `PhononPacket`s at a point when triggered. Contains customisable volume, direction, etc.

`PhononListener2D` : Picks up `PhononPacket`s that collide with it and processes the audio.

`PhononMaterial` : Material resource that can be applied to 2D sprites and tilemaps, which defines how that material interacts with sound. E.g. 'foam' as a preset would have a high absorption property.

## Architecture
<img width="3055" height="2160" alt="image" src="https://github.com/user-attachments/assets/f54f17ad-3cd4-4292-b3bb-5823fd3fdc81" />
