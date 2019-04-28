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

#ifndef FILE_DOWNLOAD_H
#define FILE_DOWNLOAD_H

#include <string>

//Class that will download file asynchronously.
//Currently it only supports download one file at a time,
//but curl_multi function supports downloading many files in parallel.
class FileDownload {
public:
	enum DownloadStatus {
		NONE,
		DOWNLOADING,
		FINISHED,
		DOWNLOAD_ERROR,
		CANCELLED,
	} downloadStatus;

	//Download progress, in 0-1.
	float downloadProgress;

	std::string downloadURL, downloadDestination;

private:
	void *curl_, *curlm_, *dest_;

public:
	FileDownload();
	FileDownload(const FileDownload& other) = delete;
	~FileDownload();

	//Method that will perform download progress update.
	void perform();

	//Method that will cancel current download task.
	void cancel();

	//Method that will start downloading a file.
	//url: The file to download.
	//destination: The destination file or path where the file will be downloaded to.
	//isPath: Specifies destination is path or not.
	//NOTE: This function only works when currently it's not downloading.
	void downloadFile(const std::string &url, const std::string &destination, bool isPath);

	//Method that will wait until file downloading finished.
	//Returns: True if it succeeds without errors or user interrupt. Check 'downloadStatus' for more info.
	//NOTE: This function only works when currently it's downloading.
	bool waitUntilFinished();
};

#endif
