Built-in commands
=================

## General commands

These commands are available to all players.

### `/help`

Lists all available commands.

### `/info`

Shows information about your client; currently, only the protocol version and extensions.

### `/teleport`

**Arguments:**

- `<x> <y> <z>`
- `[who] <x> <y> <z>`

Teleports you to a given player or specified coordinates.

Ops can specify `who` will be teleported.

### `/online`

Lists the players currently online.

### `/version`

Shows information about the version of Thirty currently in use.

## Operator commands

These commands are only usable by players who are currently operators.

### Name lists (`/ban`, `/ban-ip`, `/op`, `/whitelist`)

There are a few commands that are combined here into one documentation section because they work identically: they each manage a list, only differing by which list they manage.
`ban` manages player name bans, `ban-ip` manages player IP bans, `op` manages opped player names, `whitelist` manages whitelisted player names.
`whitelist` requires that the server whitelist be enabled in the [server settings](settings.md).

**Arguments:**

- `add <entry>`

Adds the given entry to the list.

- `remove <entry>`

Removes the given entry from the list.

- `list`

Shows the entries currently in the list.

### `/save`

Immediately saves the world to disk.

### `/env`

**Arguments:**

- `colour <sky|cloud|fog|ambient|sunlight|skybox> <r> <g> <b>`
- `colour <sky|cloud|fog|ambient|sunlight|skybox> default`

Sets the given map environment colour to the given value, or resets it to default.

- `weather <clear|rain|snow>`

Sets the map weather.
