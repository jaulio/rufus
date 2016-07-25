#include "stdafx.h"

extern "C" {
#include "rufus.h"
}

#include "TorrentDownloader.h"

#define INTMAX_MAX   INT64_MAX

///////////////////////////////////////////
//#define BOOST_CB_DISABLE_DEBUG
#undef malloc
#undef free
#include <libtorrent/session.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_info.hpp>

#include "boost/asio/impl/src.hpp"
#include "boost/asio/ssl/impl/src.hpp"

namespace lt = libtorrent;

void DownloadTorrent(const CStringA appPath)
{
    lt::session ses;

    lt::add_torrent_params atp;
    atp.url = "https://torrents.endlessm.com/torrents/eos-i386-i386-base-2.6.5-full+installer+creator.torrent";
	//atp.url = "https://torrents.endlessm.com/torrents/eos-i386-i386-en-2.6.5-full+installer+creator.torrent";
	//atp.url = "https://archive.org/download/testmp3testfile/testmp3testfile_archive.torrent";
    atp.save_path = appPath; // save in current dir

    lt::error_code error_code;
    lt::torrent_handle h = ses.add_torrent(atp, error_code);
	
	uprintf("File path %s", h.save_path().c_str());

    for (;;) {
        std::vector<lt::alert*> alerts;
        ses.pop_alerts(&alerts);

        for (lt::alert const* a : alerts) {
            //std::cout << a->message() << std::endl;
            uprintf("type=[%d] {%s}", a->type(), a->message().c_str());

			if (lt::alert_cast<lt::metadata_received_alert>(a)) {
				boost::shared_ptr<const lt::torrent_info> torrentInfo = h.torrent_file();
				int numberOfFiles = torrentInfo->num_files();
				uprintf("Number of file in torrent %d", numberOfFiles);
				const lt::file_storage &files = torrentInfo->files();
				
				for (int index = 0; index < numberOfFiles; index++) {
					uprintf("File at index %d: '%s'", index, files.file_name(index).c_str());
				}

				// RADU: add more validation on number of files, file names?
				std::vector<int> file_priorities = h.file_priorities();
				// RADU: for now we use predefined indexes in the list of files, maybe detect files based on filename?
				file_priorities[0] = 1;		// don't download Windows binary

				//file_priorities[1] = 255;	// download full image
				//file_priorities[2] = 255;	// download full image asc file
				file_priorities[1] = 0;		// don't download full image
				file_priorities[2] = 1;		// don't download full image asc file

				file_priorities[3] = 0;	// don't download eos-image-keyring.gpg
				file_priorities[4] = 0;	// download installer image
				file_priorities[5] = 0;	// download installer image asc file

				h.prioritize_files(file_priorities);
			}

			// if we receive the finished alert or an error, we're done
            if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                goto done;
            }
            if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                goto done;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
done:
    std::cout << "done, shutting down" << std::endl;
}


///////////////////////////////////////////