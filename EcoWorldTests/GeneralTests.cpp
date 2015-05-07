#include "CppUnitTest.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;


#include <thread>
#include "Game.h"

namespace EcoWorldTests
{
	TEST_CLASS(GeneralTests)
	{
	public:
		static void runGame()
		{
			game->run();
		}

		TEST_METHOD(LaunchAndQuit)
		{
			// Lance le jeu dans un thread externe
			game = new Game();
			gameState.getGameConfig().texturesQuality = GameConfiguration::ETQ_VERY_LOW;
			thread runThread(runGame);

			// Attends quelques secondes que le device s'initialise
			Sleep(2000);

#ifndef ASK_BEFORE_QUIT
			// Envoie le message de fermeture du jeu si on n'a pas de fenêtre de confirmation
			SEvent e;
			e.EventType = EET_GUI_EVENT;
			e.GUIEvent.EventType = EGET_BUTTON_CLICKED;
			e.GUIEvent.Caller = game->guiManager->guiElements.mainMenuGUI.quitterBouton;
			game->OnEvent(e);
#else
			// Quitte directement le device du jeu
			game->device->closeDevice();
#endif

			runThread.join();

			delete game;
		}
	};
}
