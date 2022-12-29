
/**
 * =============================================================================
 * Glicko SourcePawn (C)2022-2022 Lechuga.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the Glicko SourcePawn.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Sources and documentation:
 * https://rhetoricstudios.com/downloads/AbstractingGlicko2ForTeamGames.pdf
 * http://www.glicko.net/glicko/glicko.pdf
 * https://eprints.ucm.es/id/eprint/66998/1/ORY_ALONSO_Sistema_de_matchmaking_para_un_videojuego_multijugador_784051_194709458.pdf
 *
 */

#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <glicko>

/*****************************************************************
			G L O B A L   V A R S
*****************************************************************/

#define PLUGIN_VERSION "1.0"
#define MAX_OPPONENTS  3

/**
 * p | r    | RD  | g(RD)  | E(s|r,rj,RDj)
 * 1 | 1500 | 200 | 0.      | 0.
 */
PlayerInfo	 g_player = { DEFAULT_RATING, 200.0 };

/**
 * j | r    | RD  | g(RD)  |  E(s|r,rj,RDj) | outcome (s)
 * 1 | 1400 | 30  | 0.9955  | 0.639         | 1
 * 2 | 1550 | 100 | 0.9531  | 0.432         | 0
 * 3 | 1700 | 300 | 0.7242  | 0.303         | 0
 */

PlayerInfo	 g_j1	  = { 1400.0, 30.0 };
PlayerInfo	 g_j2	  = { 1550.0, 100.0 };
PlayerInfo	 g_j3	  = { 1700.0, 300.0 };

PlayerInfo	 g_Opponents[MAX_OPPONENTS];

MatchResults PlayerResults[MAX_OPPONENTS] = { Result_Win, Result_Loss, Result_Loss };

/*****************************************************************
			P L U G I N   I N F O
*****************************************************************/
public Plugin myinfo =
{
	name		= "Glicko Test",
	author		= "lechuga",
	description = "Test the formulas of Glicko's adaptation.",
	version		= PLUGIN_VERSION,
	url			= "https://github.com/lechuga16/glicko_sourcepawn"
};

/*****************************************************************
			F O R W A R D   P U B L I C S
*****************************************************************/
public void OnPluginStart()
{
	CreateConVar("sm_glicko_test", PLUGIN_VERSION, "Plugin version", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_SPONLY | FCVAR_DONTRECORD);
	RegConsoleCmd("sm_glicko_run", Command_Run);

	MatchesCompleted();
}

public Action Command_Run(int iClient, int iArgs)
{
	if (iArgs != 0)
	{
		ReplyToCommand(iClient, "Usage: sm_glicko_run");
		return Plugin_Handled;
	}

	int iSizeOP = sizeof(g_Opponents[]) + 1;
	ReplyToCommand(iClient, "Number of opponents: %d", iSizeOP);

	ReplyToCommand(iClient, "\nj1 details:");
	ReplyToCommand(iClient, "j1: g(RD) = 1 / √( 1 + (3 * q^2 * RD^2) / π^2 )");
	ReplyToCommand(iClient, "j1: g(RD) = 1 / √( 1 + (3 * %.7f^2 * %.0f^2) / %.4f^2 )", Glicko_q(), g_Opponents[0].deviation, FLOAT_PI);
	ReplyToCommand(iClient, "j1: g(RD) = %.4f", Glicko_g(g_Opponents[0]));

	ReplyToCommand(iClient, "\nj1: E(s|r, rj, RDj ) = 1 / (1 + 10^((g(RD) * (r - rj)) / 400))");
	ReplyToCommand(iClient, "j1: E(s|r,rj,RDj) = 1 / (1 + 10^(%.4f * (%.0f - %.0f)) / 400))", Glicko_g(g_Opponents[0]), g_Opponents[0].rating, g_player.rating);
	ReplyToCommand(iClient, "j1: E(s|r,rj,RDj) = %.4f", Glicko_e(g_Opponents[0], g_player));

	ReplyToCommand(iClient, "\nResume:");
	ReplyToCommand(iClient, "Player: r = %.0f | RD = %.0f | g(RD) %.5f", g_player.rating, g_player.deviation, Glicko_g(g_player));

	for (int i = 0; i <= sizeof(g_Opponents[]); i++)
	{
		ReplyToCommand(iClient, "j%d: r %.0f | RD %.0f | g(RD) %.5f | E(s|r,rj,RDj) %.5f",
					   i + 1, g_Opponents[i].rating, g_Opponents[i].deviation, Glicko_g(g_Opponents[i]), Glicko_e(g_Opponents[i], g_player));
	}

	ReplyToCommand(iClient, "\nPlayer: d^2 = 1 / (q^2 * Σ( g(RD)^2 * E(s|r,rj,RDj) * (1 - E(s|r,rj,RDj)) ))");
	ReplyToCommand(iClient, "Player: d^2 = 1 / (%.7f^2 * [\ 
					%.4f^2 * %.4f * (1 - %.4f) + \
					%.4f^2 * %.4f * (1 - %.4f) + \
					%.4f^2 * %.4f * (1 - %.4f)])",
				   Glicko_q(),
				   Glicko_g(g_Opponents[0]), Glicko_e(g_Opponents[0], g_player), Glicko_e(g_Opponents[0], g_player),
				   Glicko_g(g_Opponents[1]), Glicko_e(g_Opponents[1], g_player), Glicko_e(g_Opponents[1], g_player),
				   Glicko_g(g_Opponents[2]), Glicko_e(g_Opponents[2], g_player), Glicko_e(g_Opponents[2], g_player));

	float fSum_d;
	for (int i = 0; i <= sizeof(g_Opponents[]); i++)
	{
		fSum_d += Glicko_sum_d(g_player, g_Opponents[i]);
	}

	float fGlicko_d_Full = Glicko_d(fSum_d);
	ReplyToCommand(iClient, "Player: d = %.2f", fGlicko_d_Full);

	float fFinalRD = Glicko_FinalRD(iClient, g_player, fGlicko_d_Full);
	ReplyToCommand(iClient, "\nPlayer: RD' = %.2f", fFinalRD);

	float fSum_FinalRating;
	for (int i = 0; i <= sizeof(g_Opponents[]); i++)
	{
		fSum_FinalRating += Glicko_sum_FinalRating(iClient, g_player, g_Opponents[i], PlayerResults[i]);
	}

	float fFinalRating = Glicko_FinalRating(iClient, g_player, fGlicko_d_Full, fSum_FinalRating);
	ReplyToCommand(iClient, "Player: r' = %.2f", fFinalRating);

	return Plugin_Handled;
}

/*****************************************************************
			P L U G I N   F U N C T I O N S
*****************************************************************/
public void MatchesCompleted()
{
	g_Opponents[0] = g_j1;
	g_Opponents[1] = g_j2;
	g_Opponents[2] = g_j3;
}
