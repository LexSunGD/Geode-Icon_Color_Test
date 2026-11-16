#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

ccColor3B mezclarColores(ccColor3B colorA, ccColor3B colorB, float factor) {
    return {
        static_cast<GLubyte>(colorA.r * (1.0f - factor) + colorB.r * factor),
        static_cast<GLubyte>(colorA.g * (1.0f - factor) + colorB.g * factor),
        static_cast<GLubyte>(colorA.b * (1.0f - factor) + colorB.b * factor)
    };
}

class $modify(MiModColores, PlayLayer) {
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        if (!this->m_player1 || !this->m_effectManager) return;

        auto gm = GameManager::sharedState();

        ccColor3B playerCol1 = gm->colorForIdx(gm->getPlayerColor());
        ccColor3B playerCol2 = gm->colorForIdx(gm->getPlayerColor2());

        ccColor3B bgColor = this->m_effectManager->activeColorForIndex(1000);
        ccColor3B gColor  = this->m_effectManager->activeColorForIndex(1001);

        ccColor3B nuevoColor1 = mezclarColores(playerCol1, bgColor, 0.5f);
        ccColor3B nuevoColor2 = mezclarColores(playerCol2, gColor, 0.5f);

        this->m_player1->setColor(nuevoColor1);
        this->m_player1->setSecondColor(nuevoColor2);

        if (this->m_player2) {
            this->m_player2->setColor(nuevoColor2);
            this->m_player2->setSecondColor(nuevoColor1);
        }
    }
};
