#include "custom_display_handler.h"

namespace Noblegarden
{
    void DisplayHandler::Display(ChatHandler* handler, DisplayType type, uint32 id, uint8 bonus, bool isSecondary)
    {
        auto itemSlot = m_item_slots.at(type);
        auto player = handler->GetPlayer();

        if (auto item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, itemSlot))
        {
            if (id == 0)
            {
                ResetItem(item);

                if (isSecondary)
                {
                    item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_ALL_SPECS, 0);
                }
                else
                {
                    item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, 0);
                }

                player->SetVisibleItemSlot(itemSlot, item);
                item->ClearBonuses();
                item->SetState(ITEM_CHANGED, player);
            }
            else
            {
                if (auto itemTemplate = sObjectMgr->GetItemTemplate(id))
                {
                    // Try find regular mod
                    if (auto modifiedAppearance1 = sDB2Manager.GetItemModifiedAppearance(id, m_valid_appearance_mod_list[bonus].first))
                    {
                        ApplyModifiedAppearance(modifiedAppearance1->ID, modifiedAppearance1->ItemAppearanceID, item, player, handler, itemSlot, isSecondary);
                        return;
                    }
                    else
                    {
                        for (auto mod : m_valid_appearance_mod_list)
                        {
                            if (modifiedAppearance1 = sDB2Manager.GetItemModifiedAppearance(id, mod.first))
                            {
                                if (ApplyModifiedAppearance(modifiedAppearance1->ID, modifiedAppearance1->ItemAppearanceID, item, player, handler, itemSlot, isSecondary))
                                    return;
                            }
                        }
                    }

                    // Try find legendary mod
                    if (auto modifiedAppearance2 = sDB2Manager.GetItemModifiedAppearance(id, m_valid_appearance_mod_list[bonus].second))
                    {
                        ApplyModifiedAppearance(modifiedAppearance2->ID, modifiedAppearance2->ItemAppearanceID, item, player, handler, itemSlot, isSecondary);
                        return;
                    }
                    else
                    {
                        for (auto mod : m_valid_appearance_mod_list)
                        {
                            if (modifiedAppearance2 = sDB2Manager.GetItemModifiedAppearance(id, mod.second))
                            {
                                if (ApplyModifiedAppearance(modifiedAppearance2->ID, modifiedAppearance2->ItemAppearanceID, item, player, handler, itemSlot, isSecondary))
                                    return;
                            }
                        }

                        handler->SendSysMessage("Target item have no appearance.");
                    }
                }
                else
                {
                    handler->SendSysMessage("Target item not found.");
                }
            }

        }
        else
        {
            handler->SendSysMessage("Slot is empty.");
        }
    }


    void DisplayHandler::ResetItem(Item* item)
    {
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_SPEC_1, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_SPEC_2, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_SPEC_3, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_SPEC_4, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_1, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_2, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_3, 0);
        item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_SPEC_4, 0);
    }


    bool DisplayHandler::ApplyModifiedAppearance(int modApID, int apID, Item* item, Player* player, ChatHandler* handler, EquipmentSlots itemSlot, bool isSecondary)
    {
        if (apID == 0)
        {
            return false;
        }

        auto targetAppearance = sItemAppearanceStore.LookupEntry(apID);
        auto targetType = (DisplayType)targetAppearance->DisplayType;
        auto playerType = GetDisplayType(item);

        if (targetType == DISPLAY_TYPE_NONE || playerType == DISPLAY_TYPE_NONE)
        {
            handler->SendSysMessage("Target item is not part of equipment.");
            return false;
        }
        else
        {
            auto validSlots = m_display_map.at(playerType);
            auto itemIsValid = std::find(validSlots.begin(), validSlots.end(), targetType) != validSlots.end();

            if (!itemIsValid)
            {
                handler->SendSysMessage("Target item type is not match your item type.");
                return false;
            }
            else
            {
                ResetItem(item);

                if (isSecondary)
                {
                    item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, item->GetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS));
                    item->SetModifier(ITEM_MODIFIER_TRANSMOG_SECONDARY_APPEARANCE_ALL_SPECS, modApID);
                }
                else
                {
                    item->SetModifier(ITEM_MODIFIER_TRANSMOG_APPEARANCE_ALL_SPECS, modApID);
                }

                item->SetState(ITEM_CHANGED, player);

                player->SetVisibleItemSlot(itemSlot, item);

                return true;
            }
        }

        return false;
    }


    DisplayType DisplayHandler::GetDisplayType(Item* item)
    {
        DisplayType result = DISPLAY_TYPE_NONE;

        if (auto appearanceModifiedEntry = item->GetItemModifiedAppearance())
        {
            if (auto appearanceEntry = sItemAppearanceStore.LookupEntry(appearanceModifiedEntry->ItemAppearanceID))
            {
                result = (DisplayType)appearanceEntry->DisplayType;
            }
            else
            {
                auto temp = item->GetTemplate()->GetInventoryType();
                if (m_inv_type_to_disp_type.contains(temp))
                {
                    result = m_inv_type_to_disp_type.at(temp);
                }
            }
        }
        else
        {
            auto temp = item->GetTemplate()->GetInventoryType();

            if (m_inv_type_to_disp_type.contains(temp))
            {
                result = m_inv_type_to_disp_type.at(temp);
            }
        }

        return result;
    }
};

void AddSC_CustomDisplayHandler()
{
    Noblegarden::DisplayHandler::GetInstance();
}
