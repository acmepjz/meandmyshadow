/*
 * Copyright (C) 2019 Me and My Shadow
 *
 * This file is part of Me and My Shadow.
 *
 * Me and My Shadow is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Me and My Shadow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Me and My Shadow.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FileDownload.h"
#include "FileManager.h"
#include "Functions.h"
#include "Settings.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

using namespace std;

//Included for the downloadFile method.
#include <curl/curl.h>

#define GETCURL ((CURL*)curl_)
#define GETCURLM ((CURLM*)curlm_)
#define GETDEST ((FILE*)dest_)

//Method used by curl to copy blocks of data.
static size_t writeData(void *ptr, size_t size, size_t nmemb, FILE *stream){
	return fwrite(ptr, size, nmemb, stream);
}

//Method used by curl to show download progress.
static int xferInfo(FileDownload *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	if (dltotal > 0) {
		clientp->downloadProgress = float(dlnow) / float(dltotal);
		if (clientp->downloadProgress < 0.0f) clientp->downloadProgress = 0.0f;
		if (clientp->downloadProgress > 1.0f) clientp->downloadProgress = 1.0f;
	} else {
		clientp->downloadProgress = 0.0f;
	}
#ifdef _DEBUG
	printf("Downloaded %0.2f%%     \r", clientp->downloadProgress*100.0f);
#endif
	return 0;
}

FileDownload::FileDownload()
	: downloadStatus(NONE)
	, downloadProgress(0.0f)
	, curl_(NULL), curlm_(NULL), dest_(NULL)
{
	curlm_ = curl_multi_init();
}

FileDownload::~FileDownload() {
	assert((curl_ == NULL && dest_ == NULL) || (curl_ != NULL && dest_ != NULL));

	cancel();

	curl_multi_cleanup(GETCURLM);
	curlm_ = NULL;
}

void FileDownload::perform() {
	if (curl_ && dest_) {
		downloadStatus = DOWNLOADING;

		int running_handles = 0;
		CURLMcode ret;

		do {
			ret = curl_multi_perform(GETCURLM, &running_handles);

			if (ret != CURLM_OK && ret != CURLM_CALL_MULTI_PERFORM) {
				printf("ERROR: curl_multi_perform() failed with return code %d (%s)\n", (int)ret, curl_multi_strerror(ret));
				downloadStatus = DOWNLOAD_ERROR;
			} else {
				for (;;) {
					int msgq = 0;
					CURLMsg *m = curl_multi_info_read(GETCURLM, &msgq);
					if (m == NULL) break;
					if (m->msg == CURLMSG_DONE) {
						assert(m->easy_handle == GETCURL);
						if (m->data.result == CURLE_OK) {
							downloadStatus = FINISHED;
						} else {
							printf("ERROR: Download failed with code %d (%s)\n", (int)m->data.result, curl_easy_strerror(m->data.result));
							downloadStatus = DOWNLOAD_ERROR;
						}
					}
				}
			}
		} while (ret == CURLM_CALL_MULTI_PERFORM);

		if (downloadStatus != DOWNLOADING) {
			curl_multi_remove_handle(GETCURLM, GETCURL);
			curl_easy_cleanup(GETCURL);
			curl_ = NULL;

			fclose(GETDEST);
			dest_ = NULL;

			std::string tempFileName = downloadDestination + ".downloading";

			if (downloadStatus == FINISHED) {
#ifdef _DEBUG
				printf("Successfully downloaded '%s' to '%s'\n", downloadURL.c_str(), tempFileName.c_str());
#endif
				removeFile(downloadDestination.c_str());
				if (rename(tempFileName.c_str(), downloadDestination.c_str()) != 0) {
					printf("ERROR: Failed to rename file '%s' to '%s'\n", tempFileName.c_str(), downloadDestination.c_str());
					downloadStatus = DOWNLOAD_ERROR;
				}
			} else {
				printf("ERROR: failed downloading '%s' to '%s', will try to delete the destination file\n", downloadURL.c_str(), tempFileName.c_str());
				if (!removeFile(tempFileName.c_str())) {
					printf("ERROR: failed to delete the destination file '%s'\n", tempFileName.c_str());
				}
			}
		}
	}
}

void FileDownload::cancel() {
	if (curl_ && dest_) {
		downloadStatus = CANCELLED;

		curl_multi_remove_handle(GETCURLM, GETCURL);
		curl_easy_cleanup(GETCURL);
		curl_ = NULL;

		fclose(GETDEST);
		dest_ = NULL;

		std::string tempFileName = downloadDestination + ".downloading";

#ifdef _DEBUG
		printf("Cancelled downloading '%s' to '%s', will try to delete the destination file\n", downloadURL.c_str(), tempFileName.c_str());
#endif

		if (!removeFile(tempFileName.c_str())) {
			printf("ERROR: failed to delete the destination file '%s'\n", tempFileName.c_str());
		}
	}
}

void FileDownload::downloadFile(const std::string &url, const std::string &destination, bool isPath) {
	assert((curl_ == NULL && dest_ == NULL) || (curl_ != NULL && dest_ != NULL));

	if (curl_ || dest_) return;

	downloadStatus = NONE;
	downloadProgress = 0.0f;

	if (isPath) {
		//Get the destination file name by append the file name in URL to destination path.
		downloadDestination = destination + fileNameFromPath(url, true);
	} else {
		downloadDestination = destination;
	}

	std::string tempFileName = downloadDestination + ".downloading";

	//Try to open the output file.
	dest_ = fopen(tempFileName.c_str(), "wb");
	if (dest_ == NULL) {
		printf("ERROR: Failed to create download destination file '%s'\n", tempFileName.c_str());
		downloadStatus = DOWNLOAD_ERROR;
		return;
	}

	//Init curl_easy
	curl_ = curl_easy_init();

	//Set the internet proxy
	string internetProxy = getSettings()->getValue("internet-proxy");
	size_t pos = internetProxy.find_first_of(":");
	if (pos != string::npos){
		curl_easy_setopt(GETCURL, CURLOPT_PROXYPORT, atoi(internetProxy.substr(pos + 1).c_str()));
		internetProxy = internetProxy.substr(0, pos);
		curl_easy_setopt(GETCURL, CURLOPT_PROXY, internetProxy.c_str());
	}

	//Append the url to addon_url if the url is relative
	if (url.find("://") == string::npos) {
		downloadURL = getSettings()->getValue("addon_url");
		size_t p = downloadURL.find_last_of("\\/");
		if (p != string::npos) downloadURL = downloadURL.substr(0, p + 1);
		downloadURL += url;
	} else {
		downloadURL = url;
	}

#ifdef _DEBUG
	printf("Downloading '%s' to '%s'\n", downloadURL.c_str(), tempFileName.c_str());
#endif

	curl_easy_setopt(GETCURL, CURLOPT_URL, downloadURL.c_str());
	curl_easy_setopt(GETCURL, CURLOPT_WRITEFUNCTION, writeData);
	curl_easy_setopt(GETCURL, CURLOPT_WRITEDATA, dest_);
	curl_easy_setopt(GETCURL, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(GETCURL, CURLOPT_MAXREDIRS, 8);
	curl_easy_setopt(GETCURL, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(GETCURL, CURLOPT_XFERINFOFUNCTION, xferInfo);
	curl_easy_setopt(GETCURL, CURLOPT_XFERINFODATA, this);

	//Add the curl_easy to curl_multi
	curl_multi_add_handle(GETCURLM, GETCURL);

	//Set the status to downloading.
	downloadStatus = DOWNLOADING;

	//Perform the progress update for the first time.
	perform();
}

bool FileDownload::waitUntilFinished() {
	while (curl_ && dest_) {
		perform();
		SDL_Delay(25);
	}
	return downloadStatus == FINISHED;
}
