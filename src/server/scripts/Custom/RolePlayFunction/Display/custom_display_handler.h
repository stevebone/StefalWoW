#include "Define.h"
#include "Base32.h"
#include "Chat.h"
#include "Item.h"
#include "ItemBonusMgr.h"
#include "ItemEnchantmentMgr.h"
#include "Player.h"

namespace Noblegarden
{
    enum DisplayType : uint32 {
        DISPLAY_TYPE_HEAD = 0,
        DISPLAY_TYPE_SHOULDERS = 1,
        DISPLAY_TYPE_SHIRT = 2,
        DISPLAY_TYPE_CHEST = 3,
        DISPLAY_TYPE_WAIST = 4,
        DISPLAY_TYPE_PANTS = 5,
        DISPLAY_TYPE_BOOTS = 6,
        DISPLAY_TYPE_WRISTS = 7,
        DISPLAY_TYPE_HANDS = 8,
        DISPLAY_TYPE_BACK = 9,
        DISPLAY_TYPE_TABARD = 10,
        DISPLAY_TYPE_MAIN = 11,
        DISPLAY_TYPE_RANGED = 12,
        DISPLAY_TYPE_SHIELD = 13,
        DISPLAY_TYPE_OFF = 15,
        DISPLAY_TYPE_NONE = 999,
    };

    #ifndef NOBLEGARDEN_DISPLAY_HANDLER
    #define NOBLEGARDEN_DISPLAY_HANDLER
    class DisplayHandler
    {
    // Init
    public:
        DisplayHandler()
        {

        }

        static DisplayHandler& GetInstance()
        {
            static DisplayHandler instance;
            return instance;
        }

        DisplayHandler(DisplayHandler const&) = delete;
        void operator=(DisplayHandler const&) = delete;

    // API
    public:
        void Display(ChatHandler* handler, DisplayType type, uint32 id, uint8 bonus, bool isSecondary);

    private:
        void ResetItem(Item* item);
        bool ApplyModifiedAppearance(int modApID, int apID, Item* item, Player* player, ChatHandler* handler, EquipmentSlots itemSlot, bool isSecondary);
        DisplayType GetDisplayType(Item* item);

    // Datatables
    private:
        std::map<EquipmentSlots, std::string> const m_valid_display_slots = {
            {EQUIPMENT_SLOT_HEAD        ,"hea"},
            {EQUIPMENT_SLOT_SHOULDERS   ,"sho"},
            {EQUIPMENT_SLOT_BODY        ,"shi"},
            {EQUIPMENT_SLOT_CHEST       ,"che"},
            {EQUIPMENT_SLOT_WAIST       ,"wai"},
            {EQUIPMENT_SLOT_LEGS        ,"leg"},
            {EQUIPMENT_SLOT_FEET        ,"fee"},
            {EQUIPMENT_SLOT_WRISTS      ,"wri"},
            {EQUIPMENT_SLOT_HANDS       ,"han"},
            {EQUIPMENT_SLOT_BACK        ,"bac"},
            {EQUIPMENT_SLOT_TABARD      ,"tab"},
            {EQUIPMENT_SLOT_MAINHAND    ,"mai"},
            {EQUIPMENT_SLOT_OFFHAND     ,"off"},
        };

        std::map<DisplayType, EquipmentSlots> const m_item_slots = {
            {DISPLAY_TYPE_HEAD       ,EQUIPMENT_SLOT_HEAD},
            {DISPLAY_TYPE_SHOULDERS  ,EQUIPMENT_SLOT_SHOULDERS},
            {DISPLAY_TYPE_SHIRT      ,EQUIPMENT_SLOT_BODY},
            {DISPLAY_TYPE_CHEST      ,EQUIPMENT_SLOT_CHEST},
            {DISPLAY_TYPE_WAIST      ,EQUIPMENT_SLOT_WAIST},
            {DISPLAY_TYPE_PANTS      ,EQUIPMENT_SLOT_LEGS},
            {DISPLAY_TYPE_BOOTS      ,EQUIPMENT_SLOT_FEET},
            {DISPLAY_TYPE_WRISTS     ,EQUIPMENT_SLOT_WRISTS},
            {DISPLAY_TYPE_HANDS      ,EQUIPMENT_SLOT_HANDS},
            {DISPLAY_TYPE_BACK       ,EQUIPMENT_SLOT_BACK},
            {DISPLAY_TYPE_TABARD     ,EQUIPMENT_SLOT_TABARD},
            {DISPLAY_TYPE_MAIN       ,EQUIPMENT_SLOT_MAINHAND},
            {DISPLAY_TYPE_RANGED     ,EQUIPMENT_SLOT_MAINHAND},
            {DISPLAY_TYPE_SHIELD     ,EQUIPMENT_SLOT_OFFHAND},
            {DISPLAY_TYPE_OFF        ,EQUIPMENT_SLOT_OFFHAND},
        };

        std::map<InventoryType, DisplayType> const m_inv_type_to_disp_type = {
            {INVTYPE_HEAD           ,DISPLAY_TYPE_HEAD},
            {INVTYPE_SHOULDERS      ,DISPLAY_TYPE_SHOULDERS},
            {INVTYPE_BODY           ,DISPLAY_TYPE_SHIRT},
            {INVTYPE_CHEST          ,DISPLAY_TYPE_CHEST},
            {INVTYPE_ROBE           ,DISPLAY_TYPE_CHEST},
            {INVTYPE_WAIST          ,DISPLAY_TYPE_WAIST},
            {INVTYPE_LEGS           ,DISPLAY_TYPE_PANTS},
            {INVTYPE_FEET           ,DISPLAY_TYPE_BOOTS},
            {INVTYPE_WRISTS         ,DISPLAY_TYPE_WRISTS},
            {INVTYPE_HANDS          ,DISPLAY_TYPE_HANDS},
            {INVTYPE_CLOAK          ,DISPLAY_TYPE_BACK},
            {INVTYPE_TABARD         ,DISPLAY_TYPE_TABARD},
            {INVTYPE_WEAPONMAINHAND ,DISPLAY_TYPE_MAIN},
            {INVTYPE_WEAPON         ,DISPLAY_TYPE_MAIN},
            {INVTYPE_WEAPONOFFHAND  ,DISPLAY_TYPE_OFF},
            {INVTYPE_SHIELD         ,DISPLAY_TYPE_SHIELD},
            {INVTYPE_RANGEDRIGHT    ,DISPLAY_TYPE_RANGED},
            {INVTYPE_RANGED         ,DISPLAY_TYPE_RANGED},
        };

        std::map<DisplayType, std::list<DisplayType>> const m_display_map = {
            {DISPLAY_TYPE_HEAD      ,{DISPLAY_TYPE_HEAD}},
            {DISPLAY_TYPE_CHEST     ,{DISPLAY_TYPE_BACK, DISPLAY_TYPE_CHEST, DISPLAY_TYPE_SHIRT, DISPLAY_TYPE_HANDS, DISPLAY_TYPE_WRISTS, DISPLAY_TYPE_BOOTS, DISPLAY_TYPE_TABARD, DISPLAY_TYPE_PANTS}},
            {DISPLAY_TYPE_SHIRT     ,{DISPLAY_TYPE_BACK, DISPLAY_TYPE_CHEST, DISPLAY_TYPE_SHIRT, DISPLAY_TYPE_HANDS, DISPLAY_TYPE_WRISTS, DISPLAY_TYPE_BOOTS, DISPLAY_TYPE_TABARD, DISPLAY_TYPE_PANTS}},
            {DISPLAY_TYPE_TABARD    ,{DISPLAY_TYPE_WAIST, DISPLAY_TYPE_CHEST, DISPLAY_TYPE_SHIRT, DISPLAY_TYPE_BOOTS, DISPLAY_TYPE_TABARD, DISPLAY_TYPE_PANTS}},
            {DISPLAY_TYPE_BACK      ,{DISPLAY_TYPE_BACK, DISPLAY_TYPE_CHEST, DISPLAY_TYPE_PANTS}},
            {DISPLAY_TYPE_WRISTS    ,{DISPLAY_TYPE_BACK, DISPLAY_TYPE_WRISTS, DISPLAY_TYPE_HANDS, DISPLAY_TYPE_SHIRT, DISPLAY_TYPE_CHEST}},
            {DISPLAY_TYPE_HANDS     ,{DISPLAY_TYPE_WRISTS, DISPLAY_TYPE_HANDS, DISPLAY_TYPE_SHIRT, DISPLAY_TYPE_CHEST}},
            {DISPLAY_TYPE_WAIST     ,{DISPLAY_TYPE_WAIST, DISPLAY_TYPE_SHIRT}},
            {DISPLAY_TYPE_SHOULDERS ,{DISPLAY_TYPE_SHOULDERS}},
            {DISPLAY_TYPE_PANTS     ,{DISPLAY_TYPE_PANTS, DISPLAY_TYPE_CHEST, DISPLAY_TYPE_BOOTS}},
            {DISPLAY_TYPE_BOOTS     ,{DISPLAY_TYPE_BOOTS, DISPLAY_TYPE_PANTS, DISPLAY_TYPE_CHEST}},
            {DISPLAY_TYPE_OFF       ,{DISPLAY_TYPE_OFF, DISPLAY_TYPE_RANGED, DISPLAY_TYPE_SHIELD, DISPLAY_TYPE_MAIN}},
            {DISPLAY_TYPE_RANGED    ,{DISPLAY_TYPE_OFF, DISPLAY_TYPE_RANGED, DISPLAY_TYPE_SHIELD, DISPLAY_TYPE_MAIN}},
            {DISPLAY_TYPE_SHIELD    ,{DISPLAY_TYPE_OFF, DISPLAY_TYPE_RANGED, DISPLAY_TYPE_SHIELD, DISPLAY_TYPE_MAIN}},
            {DISPLAY_TYPE_MAIN      ,{DISPLAY_TYPE_OFF, DISPLAY_TYPE_RANGED, DISPLAY_TYPE_SHIELD, DISPLAY_TYPE_MAIN}},
        };

        // 1 - regular, 2 - legendary
        std::pair<uint32, uint32> m_valid_appearance_mod_list[76] = {
            std::make_pair(0,    9),
            std::make_pair(1,   10),
            std::make_pair(3,   11),
            std::make_pair(4,   12),
            std::make_pair(5,   13),
            std::make_pair(6,   14),
            std::make_pair(7,   15),
            std::make_pair(8,   16),
            std::make_pair(49,  17),
            std::make_pair(150, 18),
            std::make_pair(151, 19),
            std::make_pair(152, 20),
            std::make_pair(153, 21),
            std::make_pair(154, 22),
            std::make_pair(155, 23),
            std::make_pair(156, 24),
            std::make_pair(157, 25),
            std::make_pair(158, 26),
            std::make_pair(159, 27),
            std::make_pair(160, 28),
            std::make_pair(161, 29),
            std::make_pair(162, 30),
            std::make_pair(163, 31),
            std::make_pair(164, 32),
            std::make_pair(165, 0),
            std::make_pair(166, 0),
            std::make_pair(167, 0),
            std::make_pair(168, 0),
            std::make_pair(169, 0),
            std::make_pair(170, 0),
            std::make_pair(171, 0),
            std::make_pair(172, 0),
            std::make_pair(173, 0),
            std::make_pair(174, 0),
            std::make_pair(175, 0),
            std::make_pair(176, 0),
            std::make_pair(177, 0),
            std::make_pair(178, 0),
            std::make_pair(179, 0),
            std::make_pair(180, 0),
            std::make_pair(181, 0),
            std::make_pair(182, 0),
            std::make_pair(183, 0),
            std::make_pair(184, 0),
            std::make_pair(185, 0),
            std::make_pair(186, 0),
            std::make_pair(187, 0),
            std::make_pair(188, 0),
            std::make_pair(189, 0),
            std::make_pair(190, 0),
            std::make_pair(191, 0),
            std::make_pair(192, 0),
            std::make_pair(193, 0),
            std::make_pair(194, 0),
            std::make_pair(195, 0),
            std::make_pair(196, 0),
            std::make_pair(197, 0),
            std::make_pair(198, 0),
            std::make_pair(199, 0),
            std::make_pair(200, 0),
            std::make_pair(201, 0),
            std::make_pair(202, 0),
            std::make_pair(203, 0),
            std::make_pair(204, 0),
            std::make_pair(205, 0),
            std::make_pair(206, 0),
            std::make_pair(207, 0),
            std::make_pair(208, 0),
            std::make_pair(209, 0),
            std::make_pair(210, 0),
            std::make_pair(211, 0),
            std::make_pair(212, 0),
            std::make_pair(213, 0),
            std::make_pair(214, 0),
            std::make_pair(215, 0),
            std::make_pair(216, 0),
        };
    };
    #endif
};
