/*  DreamChess
**  Copyright (C) 2006  The DreamChess project
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <SDL.h>
#include <SDL_mixer.h>

#include "debug.h"
#include "playlist.h"
#include "audio.h"
#include "theme.h"

static sound_t sounds[AUDIO_SOUNDS] = 
{
	{AUDIO_MENU_MOVE, "menu1.wav"},
	{AUDIO_MENU_CHANGE, "menu2.wav"},
	{AUDIO_MOVE, "move1.wav"}
};

static audio_music_callback_t music_callback = NULL;
static Mix_Music *music = NULL;
static playlist_t *playlist;
static int next_song;
static playlist_entry_t *current_song;

static Mix_Chunk *wav_data[AUDIO_SOUNDS];

static int sound_volume, music_volume;

static void load_sounds()
{
	int i;

	for (i = 0; i < AUDIO_SOUNDS; i++) {
		DBG_LOG("loading %s", sounds[i].filename);
		wav_data[sounds[i].id] = Mix_LoadWAV(sounds[i].filename);
		if (!wav_data[sounds[i].id]) {
			DBG_ERROR("failed to load %s", sounds[i].filename);
			exit(1);
		}
	}
}

static void free_sounds()
{
	int i;

	for (i = 0; i < AUDIO_SOUNDS; i++)
		Mix_FreeChunk(wav_data[i]);
}

static void music_finished()
{
	next_song = 1;
}

static void sample_finished(int channel)
{
	Mix_VolumeMusic(music_volume);
}

void audio_init()
{
        music_packs_t *music_packs;
	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16;
	int audio_channels = 2;
	int audio_buffers = 4096;
	music_pack_t *music_pack;

	music_packs = theme_get_music_packs();

	SDL_Init(SDL_INIT_AUDIO);

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		DBG_ERROR("unable to open audio");
	}

	chdir("sounds");
	load_sounds();
	chdir("..");

	sound_volume = MIX_MAX_VOLUME;
	music_volume = MIX_MAX_VOLUME;

	Mix_HookMusicFinished(music_finished);
	Mix_ChannelFinished(sample_finished);

	playlist = playlist_create();
	TAILQ_FOREACH(music_pack, music_packs, entries)
		playlist_add_tracks(playlist, music_pack->dir);

	/* Check for at least two songs */
	if ((TAILQ_FIRST(playlist) != TAILQ_LAST(playlist, playlist)))
	{
		current_song = TAILQ_LAST(playlist, playlist);
		next_song = 1;
	}
	else
		next_song = 0;
}

void audio_exit()
{
	Mix_CloseAudio();
	free_sounds();
	playlist_destroy(playlist);
}

void audio_poll(int title)
{
	/* Start a new song when the previous one is finished. Is also
	** triggered when going from the title-screen to in-game and back
	*/
	if ((next_song == 1) || (!title && (current_song == TAILQ_FIRST(playlist)))
		|| (title && (current_song != TAILQ_FIRST(playlist))))
	{
		if (title)
			current_song = TAILQ_FIRST(playlist);
		else {
			current_song = TAILQ_NEXT(current_song, entries);
			if (!current_song) {
				/* Go to song 2 */
				current_song = TAILQ_FIRST(playlist);
				current_song = TAILQ_NEXT(current_song, entries);
			}
		}

		next_song = 0;

		if (music)
			Mix_FreeMusic(music);

		music = Mix_LoadMUS(current_song->filename);

		if (music_callback)
			music_callback(current_song->title, current_song->artist, current_song->album);

		DBG_LOG("playing %s", current_song->filename);
		Mix_PlayMusic(music, 0);
	}
}

void audio_set_music_callback(audio_music_callback_t callback)
{
	music_callback = callback;
}

void audio_play_sound(int id)
{
	Mix_VolumeMusic(music_volume / 4);
	if (Mix_PlayChannel(0, wav_data[id], 0) == -1) {
		DBG_WARN("failed to play sound %i", id);
		Mix_VolumeMusic(music_volume);
	}
}

void audio_set_sound_volume(int vol)
{
	sound_volume = vol * MIX_MAX_VOLUME / AUDIO_MAX_VOL;
	Mix_Volume(0, sound_volume);
}

void audio_set_music_volume(int vol)
{
	music_volume = vol * MIX_MAX_VOLUME / AUDIO_MAX_VOL;
	Mix_VolumeMusic(music_volume);
}
