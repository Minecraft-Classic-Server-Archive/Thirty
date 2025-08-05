Classic Protocol Extensions support
===================================

Thirty has some support for the [Classic Protocol Extensions](https://minecraft.wiki/w/Minecraft_Wiki:Projects/wiki.vg_merge/Classic_Protocol_Extension) (CPE).

## FullCP437

Thirty can handle the full code page 437 character set.

## FastMap

The FastMap extension is supported, which provides slightly more efficient map sending for both the client and server.

## CustomBlocks

The blocks in support level 1 (the only level) are supported.
Note that BlockDefinitions is not supported, so actual blocks custom to a server are not yet available.

## TwoWayPing

Allows Thirty to determine the client's ping.

## TextColors

Custom text colours can be defined in the `colours` section of the [settings file](settings.md#colours).

## EnvColors

Custom map environment colours can be set using the [`/env colour`](commands.md#env), and can be loaded from and saved to ClassicWorld files.

## EnvWeatherType

The weather can be set using the [`/env weather`](commands.md#env) command, and can be loaded from and saved to ClassicWorld files.

## MessageTypes

Technically supported, although not used; this will be useful for scripting support in the future though.
