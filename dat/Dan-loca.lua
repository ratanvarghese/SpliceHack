-- Based on cave-fila.lua
des.level_init({ style = "solidfill", fg = " " });

des.level_flags("mazelevel", "noflip");

des.level_init({ style = "mines", fg = ".", bg = " ", smoothed=true, joined=true, walled=true })
des.replace_terrain({ region={00,00, 75,19}, fromterrain=".", toterrain="L", chance=30 })

--
des.stair("up")
des.stair("down")
--
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
des.object()
--
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
des.trap()
--
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster({ class = "E", peaceful=0 })
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
des.monster()
