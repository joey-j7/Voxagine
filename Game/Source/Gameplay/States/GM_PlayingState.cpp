#include "GM_PlayingState.h"

#include "Humanoids/Players/Player.h"

void GM_PlayingState::Start(GameManager* pGameManager)
{
	// Create the two players
	auto pPlayers = pGameManager->GetPlayers();
	
	{
		// It could be that the players already have been made. (assigned by dragging and drop)
		pPlayers[0] = (pPlayers[0]) ? pPlayers[0] : new Player(pGameManager->GetWorld());
		pPlayers[1] = (pPlayers[1]) ? pPlayers[1] : new Player(pGameManager->GetWorld());
	}

	//
	{
		
	}
	pGameManager->StartGame();
}

void GM_PlayingState::Tick(GameManager* pOwner, float fDeltaTime)
{

}

void GM_PlayingState::Exit(GameManager* pGameManager)
{
	// Disable the players
	auto pPlayers = pGameManager->GetPlayers();
	{
		pPlayers[0]->SetEnabled(false);
		pPlayers[1]->SetEnabled(false);
	}

	// TODO depend if we won of lost
	pGameManager->SetState("");
}
