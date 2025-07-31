Settings file
=============

The settings file for Thirty is an ini file, such as:

```ini
[server]
port = 25565
name = Sean test server
motd = Things will break and go boom.
public = true
offline = false
max_players = 16
whitelist = false

[map]
name = world4
width = 64
depth = 64
height = 64
generator = classic
seed = 2015

[colours]
x = 0099ff33
y = aabbccdd
```

## Location

By default, settings are loaded from `settings.ini` in the working directory of the server.
You can use the `-c` command-line parameter to specify a file to use:

```
thirty -c /path/to/custom/settings.ini
```

## Fields
### `server`

| Key                      | Default                                                 | Description                                                                                                                   |
|--------------------------|---------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------|
| **`port`**               | 25565                                                   | TCP port the server will listen on.                                                                                           |
| **`name`**               | `Unnamed server`                                        | Name of the server, shown on the server list and to connecting clients.                                                       |
| **`motd`**               | `The server owner needs to set a MotD in settings.ini.` | Message of the day, shown to connecting clients.                                                                              |
| **`max_players`**        | 8                                                       | Maximum number of players that can be connected at one time.                                                                  |
| **`public`**             | true                                                    | Whether this server will be displayed on the server list.                                                                     |
| **`offline`**            | true                                                    | If enabled, this disables name verification, and disables heartbeat entirely.                                                 |
| **`whitelist`**          | false                                                   | Whether the whitelist is enabled, preventing users not listed in `whitelist.txt` from connecting.                             |
| **`enable_old_clients`** | false                                                   | Allows clients using protocol versions than `7` (classic 0.30) to connect. Experimental; those clients may behave unreliably. |

### `map`

| Key                  | Default   | Description                                                                                                           |
|----------------------|-----------|-----------------------------------------------------------------------------------------------------------------------|
| **`name`**           | `world`   | Name of the world.                                                                                                    |
| **`width`**          | 64        | Width of the world (X axis size)                                                                                      |
| **`depth`**          | 64        | Depth of the world (Y axis size)                                                                                      |
| **`height`**         | 64        | Height of the world (Z axis size)                                                                                     |
| **`generator`**      | `classic` | The level generator to use, see [Level generators](Level-generators).                                                 |
| **`seed`**           | 0         | Seed for the random number generator used by the level generator.                                                     |
| **`image_path`**     |           | If this and `image_interval` are specified, an image of the map will be generated at this location.                   |
| **`image_interval`** | 0         | If this and `image_path` are specified, an image of the map will be generated periodically. This value is in seconds. |

### `colours`

Defines custom text colours for use with the [`TextColors`](https://wiki.vg/Classic_Protocol_Extension#TextColors) CPE extension.

The key will be the colour code, the value is an 8-digit hexadecimal number in `RRGGBBAA` format defining the colour.

### `debug`

Settings under this section are mostly only useful for developing or testing the server.
Avoid using them on actual production servers.

| Key                | Defaul  | Desription                                                                                                                                                                          |
|--------------------|---------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **`fixed_salt`**   |         | Specifies a fixed salt that will be used for client authentication. Usually, a salt is randomly generated at server startup. **Do not use this** except for debugging login issues. |
| **`disable_save`** | `false` | Disables level saving. Mostly useful for writing world generators.                                                                                                                  |