#include <thread>
#include "../memory/memory.hpp"
#include "classes/globals.hpp"
#include "../classes/render.hpp"
#include "../classes/config.hpp"

namespace hack {
	std::shared_ptr<pProcess> process;
	ProcessModule base_module;

	void loop() {
		uintptr_t localPlayer = process->read<uintptr_t>(base_module.base + config::dwLocalPlayer);
		if (!localPlayer) return;

		int localTeam = process->read<int>(localPlayer + config::m_iTeamNum);

		view_matrix_t view_matrix = process->read<view_matrix_t>(base_module.base + config::dwViewMatrix);

		Vector3 localOrigin = process->read<Vector3>(localPlayer + config::m_vecOrigin);

		uintptr_t entity_list = process->read<uintptr_t>(base_module.base + config::dwEntityList);

		for (int i = 1; i < 32; i++) {
			uintptr_t list_entry = process->read<uintptr_t>(entity_list + (8 * (i & 0x7FFF) >> 9) + 16);
			if (!list_entry) continue;
			uintptr_t player = process->read<uintptr_t>(list_entry + 120 * (i & 0x1FF));

			if (!player) continue;

			int playerHealth = process->read<int>(player + config::dwPawnHealth);
			if (playerHealth <= 0 || playerHealth > 100) continue;

			// https://github.com/UnnamedZ03/CS2-external-base/blob/main/source/CSSPlayer.hpp#L132
			std::uint32_t playerpawn = process->read<std::uint32_t>(player + config::dwPlayerPawn);

			uintptr_t list_entry2 = process->read<uintptr_t>(entity_list + 0x8 * ((playerpawn & 0x7FFF) >> 9) + 16);
			if (!list_entry2) continue;
			uintptr_t pCSPlayerPawn = process->read<uintptr_t>(list_entry2 + 120 * (playerpawn & 0x1FF));

			if (pCSPlayerPawn == localPlayer) continue;

			int playerTeam = process->read<int>(player + config::m_iTeamNum);

			std::string playerName = "Invalid Name";
			DWORD64 playerNameAddress = process->read<DWORD64>(player + config::dwSanitizedName);
			if (playerNameAddress) {
				char buf[256];
				process->read_raw(playerNameAddress, buf, sizeof(buf));
				playerName = std::string(buf);
			}

			// https://github.com/UnnamedZ03/CS2-external-base/blob/main/source/CSSPlayer.hpp#L132
			Vector3 origin = process->read<Vector3>(pCSPlayerPawn + config::m_vecOrigin);

			//if ((localOrigin - origin).length2d() > 1000) return;

			Vector3 head;
			head.x = origin.x;
			head.y = origin.y;
			head.z = origin.z + 75.f;

			Vector3 screenpos = origin.world_to_screen(view_matrix);
			Vector3 screenhead = head.world_to_screen(view_matrix);

			float height = screenpos.y - screenhead.y;
			float width = height / 2.0f;
			if (screenpos.z >= 0.01f) {
				render::DrawBorderBox(
					g::hdcBuffer,
					screenhead.x - width / 2,
					screenhead.y,
					width,
					height,
					(localTeam == playerTeam ? RGB(75, 175, 75) : RGB(175, 75, 75))
				);
				// Draw the outer black box (top and left side of the red box)
				render::DrawBorderBox(g::hdcBuffer, screenhead.x - width / 2 - 1, screenhead.y - 1, width + 2, height + 2, RGB(0, 0, 0));

				// Draw the inner black box (bottom and right side of the red box)
				render::DrawBorderBox(g::hdcBuffer, screenhead.x - width / 2 + 1, screenhead.y + 1, width - 2, height - 2, RGB(0, 0, 0));
				render::DrawBorderBox(
					g::hdcBuffer,
					screenhead.x - (width / 2 + 5),
					screenhead.y + (height * (100 - playerHealth) / 100),
					2,
					height - (height * (100 - playerHealth) / 100),
					RGB(
						(255 - playerHealth),
						(55 + playerHealth * 2),
						50
					)
				);

				render::RenderText(
					g::hdcBuffer,
					screenhead.x + (width / -5 + -50),
					screenhead.y + (height / 50 + 1),
					playerName.c_str(),
					RGB(175, 175, 175),
					15
				);

				render::RenderText(
					g::hdcBuffer,
					screenhead.x + (width / 2 + 5),
					screenhead.y + 10,
					(std::to_string(playerHealth) + "hp").c_str(),
					RGB(
						(255 - playerHealth),
						(55 + playerHealth * 2),
						75
					),
					10
				);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}