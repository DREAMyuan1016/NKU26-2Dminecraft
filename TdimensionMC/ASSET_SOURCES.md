# Asset References

These were the online references checked for the prototype art direction:

- Player inspiration: Kenney platformer character preview on Wikimedia Commons
  - https://commons.wikimedia.org/wiki/File:Kenney.nl_platformer_characters_-_player_vector.svg
- Dirt block reference: Terraria Dirt Block page on Terraria Wiki
  - https://terraria.wiki.gg/wiki/Dirt_Block
- Alternate dirt block sprite reference: Terraria Fandom Dirt Block page
  - https://terraria.fandom.com/wiki/Dirt_Block

Current status:

- The prototype uses code-drawn placeholder visuals inside `GameWidget` and `DirtBlock`.
- Direct asset downloading from the current environment hit remote rate-limit / network restrictions.
- The gameplay code is structured so local PNG assets can be swapped in later without changing the world logic.
