#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// Blends two RGB colors based on a factor (0.0f to 1.0f)
ccColor3B blendColors(ccColor3B colorA, ccColor3B colorB, float factor) {
    return {
        static_cast<GLubyte>(colorA.r * (1.0f - factor) + colorB.r * factor),
        static_cast<GLubyte>(colorA.g * (1.0f - factor) + colorB.g * factor),
        static_cast<GLubyte>(colorA.b * (1.0f - factor) + colorB.b * factor)
    };
}

// Inverts an RGB color (chromatic negative: black <-> white, etc.)
ccColor3B invertRGBColor(ccColor3B color) {
    return {
        static_cast<GLubyte>(255 - color.r),
        static_cast<GLubyte>(255 - color.g),
        static_cast<GLubyte>(255 - color.b)
    };
}

// Applies primary and secondary colors to all icon/vehicle sprites of a PlayerObject
void applyPlayerColors(PlayerObject* player, ccColor3B primaryColor, ccColor3B secondaryColor) {
    if (!player) return;

    player->setColor(primaryColor);
    player->setSecondColor(secondaryColor);

    if (player->m_iconSprite) player->m_iconSprite->setColor(primaryColor);
    if (player->m_iconSpriteSecondary) player->m_iconSpriteSecondary->setColor(secondaryColor);
    if (player->m_vehicleSprite) player->m_vehicleSprite->setColor(primaryColor);
    if (player->m_vehicleSpriteSecondary) player->m_vehicleSpriteSecondary->setColor(secondaryColor);
}

class $modify(IconColorPlayLayer, PlayLayer) {
    struct Fields {
        bool wasModEnabled = true;
    };

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!this->m_player1 || !this->m_effectManager) return;

        auto mod = Mod::get();
        auto gm = GameManager::sharedState();

        // Original garage colors selected by the player
        ccColor3B garageColor1 = gm->colorForIdx(gm->getPlayerColor());
        ccColor3B garageColor2 = gm->colorForIdx(gm->getPlayerColor2());

        bool isModEnabled = mod->getSettingValue<bool>("enable-mod");

        // Restore original colors immediately if the mod was turned off mid-level
        if (!isModEnabled) {
            if (m_fields->wasModEnabled) {
                applyPlayerColors(this->m_player1, garageColor1, garageColor2);
                if (this->m_player2) {
                    applyPlayerColors(this->m_player2, garageColor2, garageColor1);
                }
                m_fields->wasModEnabled = false;
            }
            return;
        }

        m_fields->wasModEnabled = true;

        // Read remaining mod settings
        bool combineColor = mod->getSettingValue<bool>("combine-color");
        bool flipColor    = mod->getSettingValue<bool>("flip-color");
        bool invertColor  = mod->getSettingValue<bool>("invert-color");

        // Current level colors (1000 = BG, 1001 = Ground/G)
        ccColor3B bgColor     = this->m_effectManager->activeColorForIndex(1000);
        ccColor3B groundColor = this->m_effectManager->activeColorForIndex(1001);

        // Apply "Flip Color" (swap BG and Ground assignment)
        ccColor3B targetLevelColor1 = flipColor ? groundColor : bgColor;
        ccColor3B targetLevelColor2 = flipColor ? bgColor : groundColor;

        // Apply "Combine Color" (50% blend with garage colors vs exact level colors)
        ccColor3B finalColor1;
        ccColor3B finalColor2;

        if (combineColor) {
            finalColor1 = blendColors(garageColor1, targetLevelColor1, 0.5f);
            finalColor2 = blendColors(garageColor2, targetLevelColor2, 0.5f);
        } else {
            finalColor1 = targetLevelColor1;
            finalColor2 = targetLevelColor2;
        }

        // Apply "Invert Color" (RGB negative)
        if (invertColor) {
            finalColor1 = invertRGBColor(finalColor1);
            finalColor2 = invertRGBColor(finalColor2);
        }

        // Apply final colors to Player 1
        applyPlayerColors(this->m_player1, finalColor1, finalColor2);

        // Apply final colors to Player 2 (Dual mode)
        if (this->m_player2) {
            applyPlayerColors(this->m_player2, finalColor2, finalColor1);
        }
    }
};
