import os
import sys
import re

filePath = "./Worlds/Debug/EXAMPLE_World.wld"
file = open(filePath, 'r')

filesDatabase = {
    "Content/aim.vox" :                                                                                                         "Content/Character_Models/Player/Aim.vox",
    "Content/Levels/Fishing Village/Fishing Village Beat 1/SetDressing_25.wld" :                                                "Content/Worlds/Fishing_Village/Fishing_Village_Beat1.wld",
    "Content/UI/Level Select/Level1_Background.png" :                                                                           "Content/UI_Art/Level_Select_Sprites/Background_Level1.png",
    "Content/All Joost's levels/Moment1ValleyPathToCastleV17.wld" :                                                             "Content/Worlds/Castle/Valley_Path_To_Castle_Beat1.wld",
    "Content/UI/Level Select/Level_2Background.png" :                                                                           "Content/UI_Art/Level_Select_Sprites/Background_Level2.png",
    "Content/UI/Level Select/Background.png" :                                                                                  "Content/UI_Art/Level_Select_Sprites/Background.png",
    "Content/UI/Level Select/Loadout.png" :                                                                                     "Content/UI_Art/Level_Select_Sprites/Loadout.png",
    "Content/UI/Level Select/arrows-vector-outline.png" :                                                                       "Content/UI_Art/Level_Select_Sprites/Arrows_Vector_Outline.png",
    "Content/Textures/Sand Texture.png" :                                                                                       "Content/Textures/Sand.png",
    "Content/MainCharacterWeaponMale.vox" :                                                                                     "Content/Models/Projectiles/Main_Character_Weapon_Male.vox",
    "Content/Models/Assets/Obstacle_Wall_Long_Long_CastleWall/Obstacle_Wall_Long_Long_Foundation.vox" :                         "Content/Enviourment_Models/Obstacle_Wall_Long_Long_CastleWall/Obstacle_Wall_Long_Long_Foundation.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_1.vox" :                   "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_1.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_2.vox" :                   "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_2.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_3.vox" :                   "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_3.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_4.vox" :                   "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Bamboo_Parts/Obstacle_Pillar_Small_Tall_Bamboo_4.vox",
    "Content/Tiles/River/Big_river_modulair_pieces-1.vox" :                                                                     "Content/Enviourment_Models/River_Tiles/Big_River_Modulair_Pieces_1.vox",
    "Content/SmallHouse_V4.vox" :                                                                                               "Content/Enviourment_Models/Buildings/Small_House_V4.vox",
    "Content/SmallHouse_V1_07.vox" :                                                                                            "Content/Enviourment_Models/Buildings/Small_House_V1.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Short_Toro_Parts/Obstacle_Pillar_Small_Short_Toro_2.vox" :                     "Content/Enviourment_Models/Obstacle_Pillar_Small_Short_Toro_Parts/Obstacle_Pillar_Small_Short_Toro_2.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Short_Toro_Parts/Obstacle_Pillar_Small_Short_Toro_1.vox" :                     "Content/Enviourment_Models/Obstacle_Pillar_Small_Short_Toro_Parts/Obstacle_Pillar_Small_Short_Toro_1.vox",
    "Content/Models/Assets/Obstacle_BoatBroken.vox" :                                                                           "Content/Enviourment_Models/Obstacle_Boat_Broken.vox",
    "Content/Models/Assets/Obstacle_Wall_Medium_Short_WoodFence_Parts/Obstacle_Wall_Medium_Short_WoodFence_End.vox" :           "Content/Enviourment_Models/Obstacle_Wall_Medium_Short_WoodFence_Parts/Obstacle_Wall_Medium_Short_WoodFence_End.vox",
    "Content/miko.vox" :                                                                                                        "Content/Character_Models/Player/Main_Char_Female_Idle.anim.vox",
    "Content/Models/Assets/Obstacle_Wall_Medium_Short_Stone/Obstacle_Wall_Medium_Short_Stone.vox" :                             "Content/Enviourment_Models/Obstacle_Wall_Medium_Short_Stone/Obstacle_Wall_Medium_Short_Stone.vox",
    "Content/Models/Buildings/Wooden_Gate/Wooden_Gate.vox" :                                                                    "Content/Enviourment_Models/Buildings/Wooden_Gate/Wooden_Gate.vox",
    "Content/Models/Assets/From Discord & more/Darkwood_wall_adjusted.vox" :                                                    "Content/Enviourment_Models/Darkwood_Wall_Adjusted.vox",
    "Content/Models/Assets/Obstacle_Rectangular_Medium_Medium_Komainu/Obstacle_Rectangular_Medium_Medium_KomainuClosed.vox" :   "Content/Enviourment_Models/Obstacle_Rectangular_Medium_Medium_Komainu/Obstacle_Rectangular_Medium_Medium_KomainuClosed.vox",
    "Content/Models/Assets/Obstacle_Rectangular_Medium_Medium_Komainu/Obstacle_Rectangular_Medium_Medium_KomainuOpen.vox" :     "Content/Enviourment_Models/Obstacle_Rectangular_Medium_Medium_Komainu/Obstacle_Rectangular_Medium_Medium_KomainuOpen.vox",
    "Content/Models/Road/Cobblestone_Road_P9.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P9.vox",
    "Content/Models/Road/Cobblestone_Road_P6.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P6.vox",
    "Content/Models/Road/Cobblestone_Road_P2.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P2.vox",
    "Content/Models/Road/Cobblestone_Road_P4.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P4.vox",
    "Content/Models/Road/Cobblestone_Road_P1.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P1.vox",
    "Content/Models/Road/Cobblestone_Road_P5.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P5.vox",
    "Content/Models/Road/Cobblestone_Road_P3.vox" :                                                                             "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P3.vox",
    "Content/Models/Assets/From Discord & more/Big_tree_tall_1.vox" :                                                           "Content/Enviourment_Models/Big_Tree/Big_Tree_Tall_1.vox",
    "Content/Tiles/River/Big_river_modulair_pieces-0.vox" :                                                                     "Content/Enviourment_Models/River_Tiles/Big_River_Modulair_Pieces_0.vox",
    "Content/Models/Assets/Obstacle_Wall_Medium_Short_WoodFence_Parts/Obstacle_Wall_Medium_Short_WoodFence.vox" :               "Content/Enviourment_Models/Obstacle_Wall_Medium_Short_WoodFence_Parts/Obstacle_Wall_Medium_Short_WoodFence.vox",
    "Content/Models/Assets/Obstacle_Gate_Medium_Torii/Obstacle_Gate_Medium_Torii_2.vox" :                                       "Content/Enviourment_Models/Obstacle_Gate_Medium_Torii/Obstacle_Gate_Medium_Torii_2.vox",
    "Content/Models/Assets/Obstacle_Gate_Medium_Torii/Obstacle_Gate_Medium_Torii_1.vox" :                                       "Content/Enviourment_Models/Obstacle_Gate_Medium_Torii/Obstacle_Gate_Medium_Torii_1.vox",
    "Content/Models/Assets/From Discord & more/Big_tree-2.vox" :                                                                "Content/Enviourment_Models/Big_Tree/Big_Tree_2.vox",
    "Content/Models/Assets/From Discord & more/Big_tree-1.vox" :                                                                "Content/Enviourment_Models/Big_Tree/Big_Tree_1.vox",
    "Content/Models/Assets/From Discord & more/Big_tree-0.vox" :                                                                "Content/Enviourment_Models/Big_Tree/Big_Tree_0.vox",
    "Content/Models/Assets/Obstacle_Boat.vox" :                                                                                 "Content/Enviourment_Models/Obstacle_Boat.vox",
    "Content/Grey_full.png" :                                                                                                   "Content/Textures/Grey.png",
    "Content/Models/Building Blocks/BB_Wall_Hx15Wx10_Lx80.vox" :                                                                "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx15Wx10_Lx80.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx60_Wx10_Lx80.vox" :                                                               "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx60_Wx10_Lx80.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx100_Wx10_Lx120.vox" :                                                             "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx100_Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx100_Wx10_Lx80.vox" :                                                              "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx100_Wx10_Lx80.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx120_Wx10_Lx120.vox" :                                                             "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx120_Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx120_Wx10_Lx80.vox" :                                                              "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx120_Wx10_Lx80.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx15Wx10_Lx120.vox" :                                                               "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx15Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx40_Wx10_Lx120.vox" :                                                              "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx40_Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx40_Wx10_Lx80.vox" :                                                               "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx40_Wx10_Lx80.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx60_Wx10_Lx120.vox" :                                                              "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx60_Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx80_Wx10_Lx120.vox" :                                                              "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx80_Wx10_Lx120.vox",
    "Content/Models/Building Blocks/BB_Wall_Hx80_Wx10_Lx80.vox" :                                                               "Content/Enviourment_Models/Building_Blocks/BB_Wall_Hx80_Wx10_Lx80.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Medium_Medium_23x23x45h.vox" :                                              "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Medium_Medium_23x23x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Medium_Short_23x23x23h.vox" :                                               "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Medium_Short_23x23x23h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Thin_Medium_13x13x45h.vox" :                                                "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Thin_Medium_13x13x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Thin_Short_13x13x23h.vox" :                                                 "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Thin_Short_13x13x23h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Thin_Tall_13x13x90h.vox" :                                                  "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Thin_Tall_13x13x90h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Wide_Medium_43x43x45h.vox" :                                                "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Wide_Medium_43x43x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Pillar_Wide_Short_43x43x23h.vox" :                                                 "Content/Enviourment_Models/Building_Blocks/Obstacle_Pillar_Wide_Short_43x43x23h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_80x80x45h.vox" :                                                         "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_80x80x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Medium_Medium_20x20x45h.vox" :                                           "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Medium_Medium_20x20x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Medium_Short_20x20x23h.vox" :                                            "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Medium_Short_20x20x23h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Thin_Medium_10x10x45h.vox" :                                             "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Thin_Medium_10x10x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Thin_Short_10x10x23h.vox" :                                              "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Thin_Short_10x10x23h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Thin_Tall_10x10x90h.vox" :                                               "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Thin_Tall_10x10x90h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Wide_Medium_40x40x45h.vox" :                                             "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Wide_Medium_40x40x45h.vox",
    "Content/Models/Building Blocks/Obstacle_Rectangle_Wide_Short_40x40x23h.vox" :                                              "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_Wide_Short_40x40x23h.vox",
    "Content/LongNeckLady/LongNeck_EyesOpen.anim.vox" :                                                                         "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Open.anim.vox",
    "Content/LongNeckLady/LongNeck_EyesClosed.anim.vox" :                                                                       "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Closed.anim.vox",
    "Content/LongNeckLady/LongNeck_NeckExtendUp.anim.vox" :                                                                     "Content/Character_Models/Long_Neck_Lady/Long_Neck_Neck_Extend_Up.anim.vox",
    "Content/LongNeckLady/LongNeck_Anticipation_n_Attack.anim.vox" :                                                            "Content/Character_Models/Long_Neck_Lady/Long_Neck_Anticipation_Attack.anim.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_Straight.vox" :                                                                  "Content/Enviourment_Models/Pipes/Obstacle_Pipe_Straight.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe.vox" :                                                                           "Content/Enviourment_Models/Pipes/Obstacle_Pipe.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_L.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_L.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_T.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_T.vox",
    "Content/Models/Assets/Pipes/Obstacle_Boiler_End.vox" :                                                                     "Content/Enviourment_Models/Pipes/Obstacle_Boiler_End.vox",
    "Content/Models/Assets/Pipes/Obstacle_Boiler_Mid.vox" :                                                                     "Content/Enviourment_Models/Pipes/Obstacle_Boiler_Mid.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_3.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_3.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_4.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_4.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_5.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_5.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_6.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_6.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_Valve.vox" :                                                                     "Content/Enviourment_Models/Pipes/Obstacle_Pipe_Valve.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_Valve_1.vox" :                                                                   "Content/Enviourment_Models/Pipes/Obstacle_Pipe_Valve_1.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_Valve_2.vox" :                                                                   "Content/Enviourment_Models/Pipes/Obstacle_Pipe_Valve_2.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_Valve_Meter.vox" :                                                               "Content/Enviourment_Models/Pipes/Obstacle_Pipe_Valve_Meter.vox",
    "Content/Models/Assets/Pipes/Obstacle_Pipe_X.vox" :                                                                         "Content/Enviourment_Models/Pipes/Obstacle_Pipe_X.vox",
    "Content/Umbrella_Yokai-3.vox" :                                                                                            "Content/Character_Models/Lantern/Lantern_Idle.anim.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor1.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor1.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor2.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor2.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor3.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor3.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor4.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor4.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor5.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor5.vox",
    "Content/Models/Buildings/Modular_Buildings/Floor/BuildingFloor6.vox" :                                                     "Content/Enviourment_Models/Buildings/Floor/Building_Floor6.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece1.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece1.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece10.vox" :    "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece10.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece11.vox" :    "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece11.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece12.vox" :    "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece12.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece13.vox" :    "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece13.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece14.vox" :    "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece14.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece2.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece2.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece3.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece3.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece4.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece4.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece5.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece5.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece6.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece6.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece7.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece7.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece8.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece8.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/HouseStoneWithWoodWallPiece9.vox" :     "Content/Enviourment_Models/Buildings/Walls/House_Stone_With_Wood_Wall_Pieces/House_Stone_With_Wood_Wall_Piece9.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/StoneRailingOnTopOfWallPiece1.vox" :  "Content/Enviourment_Models/Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/Stone_Railing_On_Top_Of_Wall_Piece1.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/StoneRailingOnTopOfWallPiece2.vox" :  "Content/Enviourment_Models/Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/Stone_Railing_On_Top_Of_Wall_Piece2.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/StoneRailingOnTopOfWallPiece3.vox" :  "Content/Enviourment_Models/Buildings/Walls/Stone_Railing_On_Top_Of_Wall_Pieces/Stone_Railing_On_Top_Of_Wall_Piece3.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece1.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece1.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece2.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece2.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece3.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece3.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece4.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece4.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece5.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece5.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece6.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece6.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece7.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece7.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece8.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece8.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece9.vox" :            "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece9.vox",
    "Content/Models/Buildings/Modular_Buildings/Walls/Wood_With_Railing_Wall_Pieces/WoodWithRailingWallPiece10.vox" :           "Content/Enviourment_Models/Buildings/Walls/Wood_With_Railing_Wall_Pieces/Wood_With_Railing_Wall_Piece10.vox",
    "Content/Models/Buildings/Modular_Buildings/Corner_Wood_Modular_Buildings.vox" :                                            "Content/Enviourment_Models/Buildings/Corner_Wood_Modular_Buildings.vox",
    "Content/Models/Buildings/Modular_Buildings/Roofs/BuildingRoof1.vox" :                                                      "Content/Enviourment_Models/Buildings/Roofs/Building_Roof1.vox",
    "Content/Models/Buildings/Modular_Buildings/Roofs/BuildingRoof2.vox" :                                                      "Content/Enviourment_Models/Buildings/Roofs/Building_Roof2.vox",
    "Content/Models/Buildings/Modular_Buildings/Roofs/BuildingRoofSmall1.vox" :                                                 "Content/Enviourment_Models/Buildings/Roofs/Building_Roof_Small1.vox",
    "Content/Models/Buildings/Modular_Buildings/Roofs/BuildingRoofSmall2.vox" :                                                 "Content/Enviourment_Models/Buildings/Roofs/Building_Roof_Small2.vox",
    "Content/Models/Buildings/Modular_Buildings/Roofs/BuildingRoofSmall3.vox" :                                                 "Content/Enviourment_Models/Buildings/Roofs/Building_Roof_Small3.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P1.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P5.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P10.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P1.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P11.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P3.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P12.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P4.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P13.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P10.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P14.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P6.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P15.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P3.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P16.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P1.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P17.vox" :                                                                    "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P5.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P2.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P1.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P3.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P3.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P4.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P4.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P5.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P10.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P6.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P6.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P7.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P3.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P8.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P1.vox",
    "Content/Models/Road/Level_1_3/Road_Level_1_3_P9.vox" :                                                                     "Content/Enviourment_Models/Cobblestone_Road/Cobblestone_Road_P8.vox",
    "Content/Models/Assets/Obstacle_Block_Small_Foodcart.vox" :                                                                 "Content/Enviourment_Models/Obstacle_Block_Small_Foodcart.vox",
    "Content/Models/Buildings/Wooden_Gate/Wooden_Gate_Big.vox" :                                                                "Content/Enviourment_Models/Buildings/Wooden_Gate/Wooden_Gate_Big.vox",
    "Content/Models/Buildings/Shops/JapanShop_01.vox" :                                                                         "Content/Enviourment_Models/Buildings/Shops/Japan_Shop_01.vox",
    "Content/Models/Buildings/Shops/JapanShop_01_HP.vox" :                                                                      "Content/Enviourment_Models/Buildings/Shops/Japan_Shop_01_HP.vox",
    "Content/Models/Buildings/Shops/JapanShop_02.vox" :                                                                         "Content/Enviourment_Models/Buildings/Shops/Japan_Shop_02.vox",
    "Content/Models/Buildings/Shops/JapanShop_02_HP.vox" :                                                                      "Content/Enviourment_Models/Buildings/Shops/Japan_Shop_02_HP.vox",
    "Content/Models/Buildings/Shops/JapanShop_02_LP.vox" :                                                                      "Content/Enviourment_Models/Buildings/Shops/Japan_Shop_02_LP.vox",
    "Content/Mountain_filled.vox" :                                                                                             "Content/Enviourment_Models/Mountain.vox",
    "Content/Sprites/grass.png" :                                                                                               "Content/Textures/Grass.png",
    "Content/From Discord/Mountain_rock_wall-1.vox" :                                                                           "Content/Enviourment_Models/Mountain_Rock_Wall/Mountain_Rock_Wall_1.vox",
    "Content/From Discord/Mountain_rock_wall-2.vox" :                                                                           "Content/Enviourment_Models/Mountain_Rock_Wall/Mountain_Rock_Wall_2.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Straight_Foundation.vox" :                                      "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Straight_Foundation.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Straight_CastleWall_1.vox" :                                    "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Straight_CastleWall_1.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Straight_CastleWall_2.vox" :                                    "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Straight_CastleWall_2.vox",
    "Content/WatchtowerNew/WatchtowerNewBase.vox" :                                                                             "Content/Enviourment_Models/Buildings/Watchtower/Watchtower_Base.vox",
    "Content/WatchtowerNew/WatchtowerNewTop.vox" :                                                                              "Content/Enviourment_Models/Buildings/Watchtower/Watchtower_Top.vox",
    "Content/WatchtowerNew/WatchtowerNewRoofBase.vox" :                                                                         "Content/Enviourment_Models/Buildings/Watchtower/Watchtower_Roof_Base.vox",
    "Content/WatchtowerNew/WatchtowerNewRoofTop.vox" :                                                                          "Content/Enviourment_Models/Buildings/Watchtower/Watchtower_Roof_Top.vox",
    "Content/Audio/BGM/battle_normal.ogg" :                                                                                     "Content/Music/Battle_Normal.ogg",
    "Content/From Discord/big_tile.vox" :                                                                                       "Content/Enviourment_Models/Big_Tile.vox",
    "Content/MainChar_23-05_Female1.vox" :                                                                                      "Content/Character_Models/Player/Main_Char_Female_Idle.anim.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss.vox" :            "Content/Enviourment_Models/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss.vox",
    "Content/From Discord/rock_gate.vox" :                                                                                      "Content/Enviourment_Models/Mountain_Rock_Wall/Mountain_Rock_Wall_1.vox",
    "Content/Models/Rectangle&PillarObstacleVoxelBlocks/Obstacle_Rectangular_Medium_Wide_80x80x45h.vox" :                       "Content/Enviourment_Models/Building_Blocks/Obstacle_Rectangle_80x80x45h.vox",
    "Content/From Discord/15HighStaircaseByJoost.vox" :                                                                         "Content/Enviourment_Models/Staircase_15h.vox",
    "Content/From Discord/big_tile_15Thicc_EditByJoost.vox" :                                                                   "Content/Enviourment_Models/Big_Tile_Thick.vox",
    "Content/From Discord/MainCastleTower_03.vox" :                                                                             "Content/Enviourment_Models/Buildings/Main_Castle_Tower_03.vox",
    "Content/Textures/Texture.png" :                                                                                            "Content/Textures/Sand_Light.png",
    "Content/Sprites/sand.png" :                                                                                                "Content/Textures/Sand_Small.png",
    "Content/Sprites/sandSolid.png" :                                                                                           "Content/Textures/Sand_Small_Solid.png",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_1.vox" :          "Content/Enviourment_Models/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_1.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_2.vox" :          "Content/Enviourment_Models/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_2.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_3.vox" :          "Content/Enviourment_Models/Obstacle_Pillar_Medium_Medium_Cairn_Parts/Obstacle_Pillar_Medium_Medium_Cairn_Moss_3.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_3.vox" :       "Content/Enviourment_Models/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_3.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_2.vox" :       "Content/Enviourment_Models/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_2.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_1.vox" :       "Content/Enviourment_Models/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_1.vox",
    "Content/Models/Assets/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_4.vox" :       "Content/Enviourment_Models/Obstacle_Pillar_Medium_Short_YukimiDoro_Parts/Obstacle_Pillar_Medium_Short_YukimiDoro_4.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_1.vox" :                             "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_1.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_2.vox" :                             "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_2.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_3.vox" :                             "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_3.vox",
    "Content/Models/Assets/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_4.vox" :                             "Content/Enviourment_Models/Obstacle_Pillar_Small_Tall_Well/Obstacle_Pillar_Small_Tall_Well_4.vox",
    "Content/Models/Assets/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_1.vox" :      "Content/Enviourment_Models/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_1.vox",
    "Content/Models/Assets/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_2.vox" :      "Content/Enviourment_Models/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_2.vox",
    "Content/Models/Assets/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_3.vox" :      "Content/Enviourment_Models/Obstacle_Rectangular_Big_Medium_RiceCart/Obstacle_Rectangular_Big_Medium_RiceCartSpill_3.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Corner_CastleWall_1.vox" :                                      "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Corner_CastleWall_1.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Corner_CastleWall_2.vox" :                                      "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Corner_CastleWall_2.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_Corner_Foundation.vox" :                                        "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_Corner_Foundation.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_End_CastleWall_1.vox" :                                         "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_End_CastleWall_1.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_End_CastleWall_2.vox" :                                         "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_End_CastleWall_2.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_T_CastleWall_1.vox" :                                           "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_T_CastleWall_1.vox",
    "Content/Models/Assets/Obstacle_Wall_Small_CastleWall/Small_T_CastleWall_2.vox" :                                           "Content/Enviourment_Models/Obstacle_Wall_Small_CastleWall/Small_T_CastleWall_2.vox",
    "Content/Models/Assets/Obstacle_Wall_Long_Short_RedFence_Parts/Obstacle_Wall_Long_Short_RedFence_1.vox" :                   "Content/Enviourment_Models/Obstacle_Wall_Long_Short_RedFence_Parts/Obstacle_Wall_Long_Short_RedFence_1.vox",
    "Content/Models/Assets/Obstacle_Wall_Long_Short_RedFence_Parts/Obstacle_Wall_Long_Short_RedFence_2.vox" :                   "Content/Enviourment_Models/Obstacle_Wall_Long_Short_RedFence_Parts/Obstacle_Wall_Long_Short_RedFence_2.vox",
    "Content/Models/Assets/Lantern/Japanese_Lantern_V3.vox" :                                                                   "Content/Enviourment_Models/Lantern/Japanese_Lantern_V3.vox",
    "Content/Models/Assets/Lantern/Japanese_Lantern_Stick.vox" :                                                                "Content/Enviourment_Models/Lantern/Japanese_Lantern_Stick.vox",
    "Content/Models/Assets/Lantern/Japanese_Lantern_Rope.vox" :                                                                 "Content/Enviourment_Models/Lantern/Japanese_Lantern_Rope.vox",
    "Content/Models/Assets/Lantern/Japanese_Lantern_V2.vox" :                                                                   "Content/Enviourment_Models/Lantern/Japanese_Lantern_V2.vox",
    "Content/Models/Assets/Lantern/Japanese_Lantern_V1.vox" :                                                                   "Content/Enviourment_Models/Lantern/Japanese_Lantern_V1.vox",
    "Content/LongNeckLady/CHARACTER_long_neck_lady_Neck_Move.vox" :                                                             "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Open.anim.vox",
    "Content/LongNeckLady/CHARACTER_long_neck_lady_Eyes_Closed.vox" :                                                           "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Closed.anim.vox",
    "Content/LongNeckLady/CHARACTER_long_neck_lady_Eyes_Opened.vox" :                                                           "Content/Character_Models/Long_Neck_Lady/Long_Neck_Eyes_Open.anim.vox",
    "Content/LongNeckLady/CHARACTER_long_neck_lady_Neck_Extend.vox" :                                                           "Content/Character_Models/Long_Neck_Lady/Long_Neck_Neck_Extend_Up.anim.vox",
    "Content/Mountain_environment_design_1_mountain_wall-0.vox" :                                                               "Content/Enviourment_Models/Mountain.vox",
    "Content/LongNeckLady/CHARACTER_long_neck_lady_Neck_Extend.anim.vox" :                                                      "Content/Character_Models/Long_Neck_Lady/Long_Neck_Neck_Extend_Up.anim.vox",
}

# Validate database entries
contentFolderPath = "I:/Y2018D-Y2-SplodyMcSplodeface/Game/"
for dataBaseFile in filesDatabase:
    if not os.path.isfile(contentFolderPath + filesDatabase[dataBaseFile]):
        print("Error, files " + filesDatabase[dataBaseFile] + " does not excist.")
        sys.exit()

matchCount = 0
replacedCount = 0
addCount = 0
filesToAdd = []

newText = ""
for line in file:
    matchCount += line.count("Content/")
    if "Content/" in line:
        resourceStrings = re.findall('"Content\/([^"]*)"', line)
        for resourceString in resourceStrings:
            resourceString = "Content/" + resourceString
            # File should be replaced
            if resourceString in filesDatabase:
                replacedCount += 1
                line = line.replace(resourceString, filesDatabase[resourceString])
                print("Replaced: " + resourceString + " with: " + filesDatabase[resourceString])
            
            # File should be added to the database
            elif resourceString not in filesDatabase.values():
                addCount += 1
                filesToAdd.append(resourceString)
            
            # File is correct
            else:
                print("Skipped: " + resourceString)

    newText += line
file.close()

#Process files to replace
file = open(filePath, 'w')
file.write(newText)
file.close

# Process files to add
filesToAdd = list(dict.fromkeys(filesToAdd))
for fileToAdd in filesToAdd:
    print("Add: \"" + fileToAdd + "\" : \"Content/\",")

print("Database is correct!")
print("Total matches: " + str(matchCount) + " of which " + str(replacedCount) + " have been replaced, and " + str(addCount) + " should be added.")
if addCount == 0:
    print("Done!")