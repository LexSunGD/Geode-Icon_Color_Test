#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <string>

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

// Resolves the target color for a single channel based on user settings
ccColor3B resolveChannelColor(
    const std::string& copyMode,
    ccColor3B originalColor,
    ccColor3B bgColor,
    ccColor3B groundColor,
    bool combineColor,
    bool invertColor
) {
    ccColor3B resultColor = originalColor;

    // Only copy/blend if copyMode is not "None"
    if (copyMode == "BG" || copyMode == "G") {
        ccColor3B levelColor = (copyMode == "BG") ? bgColor : groundColor;

        if (combineColor) {
            resultColor = blendColors(originalColor, levelColor, 0.5f);
        } else {
            resultColor = levelColor;
        }
    }

    // Invert Color applies to ALL channels, even if copyMode is "None"
    if (invertColor) {
        resultColor = invertRGBColor(resultColor);
    }

    return resultColor;
}

// Applies primary, secondary, and glow colors to all sprites of a PlayerObject
void applyPlayerColors(
    PlayerObject* player,
    ccColor3B primaryColor,
    ccColor3B secondaryColor,
    ccColor3B glowColor,
    bool hasGlow
) {
    if (!player) return;

    player->setColor(primaryColor);
    player->setSecondColor(secondaryColor);

    if (player->m_iconSprite) player->m_iconSprite->setColor(primaryColor);
    if (player->m_iconSpriteSecondary) player->m_iconSpriteSecondary->setColor(secondaryColor);
    if (player->m_vehicleSprite) player->m_vehicleSprite->setColor(primaryColor);
    if (player->m_vehicleSpriteSecondary) player->m_vehicleSpriteSecondary->setColor(secondaryColor);

    if (hasGlow) {
        player->m_glowColor = glowColor;
        player->updateGlowColor();
    }
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

        // Original garage colors for Player 1
        ccColor3B p1OrigCol1 = gm->colorForIdx(gm->getPlayerColor());
        ccColor3B p1OrigCol2 = gm->colorForIdx(gm->getPlayerColor2());
        ccColor3B origGlow   = gm->colorForIdx(gm->getPlayerGlowColor());
        bool hasGlow         = gm->getPlayerGlow();

        // In Dual mode, Player 2 naturally swaps Color 1 and Color 2
        ccColor3B p2OrigCol1 = p1OrigCol2;
        ccColor3B p2OrigCol2 = p1OrigCol1;

        bool isModEnabled = mod->getSettingValue<bool>("enable-mod");

        // Restore original garage colors immediately if the mod was disabled mid-level
        if (!isModEnabled) {
            if (m_fields->wasModEnabled) {
                applyPlayerColors(this->m_player1, p1OrigCol1, p1OrigCol2, origGlow, hasGlow);
                if (this->m_player2) {
                    applyPlayerColors(this->m_player2, p2OrigCol1, p2OrigCol2, origGlow, hasGlow);
                }
                m_fields->wasModEnabled = false;
            }
            return;
        }

        m_fields->wasModEnabled = true;

        // Read mod settings
        std::string color1Copy = mod->getSettingValue<std::string>("color-1-copy");
        std::string color2Copy = mod->getSettingValue<std::string>("color-2-copy");
        std::string glowCopy   = mod->getSettingValue<std::string>("glow-copy");
        bool combineColor      = mod->getSettingValue<bool>("combine-color");
        bool invertColor       = mod->getSettingValue<bool>("invert-color");

        // Current level colors (1000 = BG, 1001 = Ground/G)
        ccColor3B bgColor     = this->m_effectManager->activeColorForIndex(1000);
        ccColor3B groundColor = this->m_effectManager->activeColorForIndex(1001);

        // Calculate final colors for Player 1
        ccColor3B p1FinalCol1 = resolveChannelColor(color1Copy, p1OrigCol1, bgColor, groundColor, combineColor, invertColor);
        ccColor3B p1FinalCol2 = resolveChannelColor(color2Copy, p1OrigCol2, bgColor, groundColor, combineColor, invertColor);
        ccColor3B finalGlow   = hasGlow
            ? resolveChannelColor(glowCopy, origGlow, bgColor, groundColor, combineColor, invertColor)
            : origGlow;

        applyPlayerColors(this->m_player1, p1FinalCol1, p1FinalCol2, finalGlow, hasGlow);

        // Calculate final colors for Player 2 (preserving its natural swapped base colors)
        if (this->m_player2) {
            ccColor3B p2FinalCol1 = resolveChannelColor(color1Copy, p2OrigCol1, bgColor, groundColor, combineColor, invertColor);
            ccColor3B p2FinalCol2 = resolveChannelColor(color2Copy, p2OrigCol2, bgColor, groundColor, combineColor, invertColor);

            applyPlayerColors(this->m_player2, p2FinalCol1, p2FinalCol2, finalGlow, hasGlow);
        }
    }
};
