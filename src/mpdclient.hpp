/*
   Vimpc
   Copyright (C) 2010 Nathan Sweetman

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   mpdclient.hpp - provides interaction with the music player daemon 
   */

#ifndef __MPC__CLIENT
#define __MPC__CLIENT

#include <mpd/client.h>

#include "library.hpp"
#include "screen.hpp"
#include "song.hpp"

namespace Ui
{
   class Screen;
}

// \todo cache all the values that we can
namespace Mpc
{
   class Client
   {
   public:
      Client(Ui::Screen const & screen);
      ~Client();

   private:
      Client(Client & client);
      Client & operator=(Client & client);

   public:
      bool GetRandom();

   public:
      void Connect(std::string const & hostname = "localhost", uint16_t port = 0);
      void Play(uint32_t playId);
      void Pause();
      void Next();
      void Previous();
      void Stop();
      void Random(bool random);
      void Single(bool single);

   public: //Queue
      uint32_t Add(Mpc::Song & song);
      void Delete(uint32_t position);
      void Clear();

   public: //Database
      void Rescan();
      void Update();

   public:
      std::string CurrentState();

   public:
      bool Connected() const;

   public:
      std::string GetCurrentSongURI() const;
      int32_t  GetCurrentSong() const;
      uint32_t TotalNumberOfSongs();

   public:
      bool SongIsInQueue(Mpc::Song & song) const;

   public:
      void DisplaySongInformation();

   public:
      template <typename Object>
      void ForEachQueuedSong(Object & object, void (Object::*callBack)(Song * const));

      template <typename Object>
      void ForEachLibrarySong(Object & object, void (Object::*callBack)(Song const * const));

   private:
      Song * CreateSong(uint32_t id, mpd_song const * const) const;

   private:
      uint32_t SecondsToMinutes(uint32_t duration) const;
      uint32_t RemainingSeconds(uint32_t duration) const;

   private:
      void CheckError();
      void DeleteConnection();

   private:
      struct mpd_connection * connection_;
      bool mutable currentSongHasChanged_;
      Ui::Screen      const & screen_;
   };

   //
   template <typename Object>
   void Client::ForEachQueuedSong(Object & object, void (Object::*callBack)(Song * const))
   {
      if (Connected() == true)
      {
         mpd_send_list_queue_meta(connection_);

         mpd_song * nextSong = mpd_recv_song(connection_);

         for (; nextSong != NULL; nextSong = mpd_recv_song(connection_))
         {
            uint32_t const     position = mpd_song_get_pos(nextSong);
            Song const * const newSong  = CreateSong(position, nextSong);
            Song * const       oldSong  = screen_.LibraryWindow().FindSong(newSong);

            if (oldSong != NULL)
            {
               oldSong->IncrementReference();
               (object.*callBack)(oldSong);
            }

            mpd_song_free(nextSong);
            delete newSong;
         }
      }
   }

   //
   template <typename Object>
   void Client::ForEachLibrarySong(Object & object, void (Object::*callBack)(Song const * const))
   {
      if (Connected() == true)
      {
         mpd_send_list_all_meta(connection_, "/");

         mpd_entity * nextEntity = mpd_recv_entity(connection_);

         for(; nextEntity != NULL; nextEntity = mpd_recv_entity(connection_))
         {
            if (mpd_entity_get_type(nextEntity) == MPD_ENTITY_TYPE_SONG)
            {
               mpd_song const * const nextSong = mpd_entity_get_song(nextEntity);

               if (nextSong != NULL)
               {
                  Song const * const newSong = CreateSong(-1, nextSong);

                  (object.*callBack)(newSong);

                  delete newSong;
               }
            }
            mpd_entity_free(nextEntity);
         }
      }
   }

}

#endif
