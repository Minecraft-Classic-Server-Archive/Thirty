// Thirty, a ClassiCube (Minecraft Classic) server
// Copyright (C) 2024 Sean Baggaley
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <stdio.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "server.h"
#include "config.h"
#include "log.h"
#include "util.h"
#include "version.h"

#define HTTP_RESPONSE_LEN 4096

static bool heartbeat_url_printed = false;

static size_t heartbeat_curl_write_func(char *ptr, size_t size, size_t nmemb, void *userdata) {
	(void) size;
	(void) nmemb;

	char *ptrs = malloc(nmemb + 1);
	memcpy(ptrs, ptr, nmemb);
	ptrs[nmemb] = '\0';

	char *str = (char *)userdata;
	strncat(str, ptrs, util_min(strlen(ptrs), HTTP_RESPONSE_LEN - strlen(str) - 1));

	free(ptrs);
	return nmemb;
}

static void *heartbeat_main(void *data) {
	(void)data;

	char response[HTTP_RESPONSE_LEN];
	memset(response, 0, HTTP_RESPONSE_LEN);

	char tmp[2048];
	CURLU *curlu = curl_url();
	curl_url_set(curlu, CURLUPART_URL, config.server.heartbeat_url, 0);
	snprintf(tmp, sizeof tmp, "port=%" PRIu16, config.server.port);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	curl_url_set(curlu, CURLUPART_QUERY, "web=True", CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "max=%d", config.server.max_players);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "public=%s", config.server.public ? "True" : "False");
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	curl_url_set(curlu, CURLUPART_QUERY, "version=7", CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "salt=%s", server.salt);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "users=%zu", server.num_spawned_clients);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "software=Thirty %s", HG_CHANGESET_HASH);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);
	snprintf(tmp, sizeof tmp, "name=%s", config.server.name);
	curl_url_set(curlu, CURLUPART_QUERY, tmp, CURLU_APPENDQUERY|CURLU_URLENCODE);

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_CURLU, curlu);
	snprintf(tmp, sizeof tmp, "Thirty %s", HG_CHANGESET_HASH);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, tmp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, heartbeat_curl_write_func);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (char *)response);

	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		long code;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

		char *surl = strstr(response, "http");
		if (code == 200L && surl != NULL) {
			if (!heartbeat_url_printed) {
				log_printf(log_info, "Server URL: %s\n", surl);
				heartbeat_url_printed = true;
			}
		}
		else {
			log_printf(log_error, "Heartbeat failed: %s\n", response);
		}
	}
	else {
		log_printf(log_error, "Failed sending heartbeat: %s\n", curl_easy_strerror(res));
	}

	curl_url_cleanup(curlu);
	curl_easy_cleanup(curl);

	return NULL;
}

void server_heartbeat(void) {
	if (config.server.offline) {
		return;
	}

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, 1);

	pthread_t thread;
	pthread_create(&thread, &attr, heartbeat_main, NULL);

	pthread_attr_destroy(&attr);
}
