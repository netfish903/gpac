/*
 *			GPAC - Multimedia Framework C SDK
 *
 *			Authors: Arash Shafiei
 *			Copyright (c) Telecom ParisTech 2000-2013
 *					All rights reserved
 *
 *  This file is part of GPAC / dashcast
 *
 *  GPAC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  GPAC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "cmd_data.h"

int dc_str_to_resolution(char * psz_str, int * p_width, int * p_height) {

	char * p_save;
	char * token = (char *) strtok_r(psz_str, "x", &p_save);

	if (!token) {
		fprintf(stderr, "Cannot parse resolution string.\n");
		return -1;
	}
	*p_width = atoi(token);

	token = (char *) strtok_r(NULL, " ", &p_save);

	if (!token) {
		fprintf(stderr, "Cannot parse resolution string.\n");
		return -1;
	}
	*p_height = atoi(token);

	return 0;
}

int dc_read_configuration(CmdData * p_cmdd) {

	int i;

	GF_Config * p_conf = p_cmdd->p_conf;

	int i_sec_count = gf_cfg_get_section_count(p_conf);

	if (i_sec_count == 0) {

		gf_cfg_set_key(p_conf, "v1.mp4", "type", "video");
		gf_cfg_set_key(p_conf, "v1.mp4", "bitrate", "400000");
		gf_cfg_set_key(p_conf, "v1.mp4", "framerate", "25");
		gf_cfg_set_key(p_conf, "v1.mp4", "width", "640");
		gf_cfg_set_key(p_conf, "v1.mp4", "height", "480");
		gf_cfg_set_key(p_conf, "v1.mp4", "codec", "libx264");

		gf_cfg_set_key(p_conf, "a1.mp4", "type", "audio");
		gf_cfg_set_key(p_conf, "a1.mp4", "bitrate", "200000");
//		gf_cfg_set_key(p_conf, "a1.mp4", "samplerate", "48000");
//		gf_cfg_set_key(p_conf, "a1.mp4", "channels", "2");
		gf_cfg_set_key(p_conf, "a1.mp4", "codec", "aac");

		i_sec_count = 2;
	}
	for (i = 0; i < i_sec_count; i++) {

		const char * psz_sec_name = gf_cfg_get_section_name(p_conf, i);

		const char * psz_type = gf_cfg_get_key(p_conf, psz_sec_name, "type");

		if (strcmp(psz_type, "video") == 0) {

			VideoData * p_vconf = malloc(sizeof(VideoData));
			strcpy(p_vconf->psz_name, psz_sec_name);
			strcpy(p_vconf->psz_codec,
					gf_cfg_get_key(p_conf, psz_sec_name, "codec"));
			p_vconf->i_bitrate = atoi(
					gf_cfg_get_key(p_conf, psz_sec_name, "bitrate"));
			p_vconf->i_framerate = atoi(
					gf_cfg_get_key(p_conf, psz_sec_name, "framerate"));
			p_vconf->i_height = atoi(
					gf_cfg_get_key(p_conf, psz_sec_name, "height"));
			p_vconf->i_width = atoi(
					gf_cfg_get_key(p_conf, psz_sec_name, "width"));

			gf_list_add(p_cmdd->p_video_lst, (void *) p_vconf);
		} else if (strcmp(psz_type, "audio") == 0) {

			AudioData * p_aconf = malloc(sizeof(AudioData));
			strcpy(p_aconf->psz_name, psz_sec_name);
			strcpy(p_aconf->psz_codec,
					gf_cfg_get_key(p_conf, psz_sec_name, "codec"));
			p_aconf->i_bitrate = atoi(
					gf_cfg_get_key(p_conf, psz_sec_name, "bitrate"));
//			p_aconf->i_samplerate = atoi(
//					gf_cfg_get_key(p_conf, psz_sec_name, "samplerate"));
//			p_aconf->i_channels = atoi(
//					gf_cfg_get_key(p_conf, psz_sec_name, "channels"));

			gf_list_add(p_cmdd->p_audio_lst, (void *) p_aconf);

		} else {
			printf("Configuration file: type %s is not supported.\n", psz_type);
		}

	}

	printf("\33[34m\33[1m");
	printf("Configurations:\n");
	for (i = 0; i < gf_list_count(p_cmdd->p_video_lst); i++) {
		VideoData * p_vconf = gf_list_get(p_cmdd->p_video_lst, i);
		printf("    id:%s\tres:%dx%d\tvbr:%d\tvfr:%d\tvcodec:%s \n",
				p_vconf->psz_name, p_vconf->i_width, p_vconf->i_height,
				p_vconf->i_bitrate, p_vconf->i_framerate, p_vconf->psz_codec);
	}

	for (i = 0; i < gf_list_count(p_cmdd->p_audio_lst); i++) {
		AudioData * p_aconf = gf_list_get(p_cmdd->p_audio_lst, i);
		printf("    id:%s\tabr:%d\tacodec:%s \n", p_aconf->psz_name,
				p_aconf->i_bitrate, /*p_aconf->i_samplerate,
				 p_aconf->i_channels,*/p_aconf->psz_codec);
	}
	printf("\33[0m");
	fflush(stdout);

	return 0;
}

void dc_cmd_data_init(CmdData * p_cmdd) {

	dc_audio_data_set_default(&p_cmdd->adata);
	dc_video_data_set_default(&p_cmdd->vdata);
	p_cmdd->p_conf = NULL;
	strcpy(p_cmdd->psz_mpd, "");
	strcpy(p_cmdd->psz_out, "");
	p_cmdd->i_exit_signal = 0;
	p_cmdd->i_live = 0;
	p_cmdd->i_seg_dur = 0;
	p_cmdd->i_frag_dur = 0;
	p_cmdd->p_audio_lst = gf_list_new();
	p_cmdd->p_video_lst = gf_list_new();

}

void dc_cmd_data_destroy(CmdData * p_cmdd) {

	gf_list_del(p_cmdd->p_audio_lst);
	gf_list_del(p_cmdd->p_video_lst);
	gf_cfg_del(p_cmdd->p_conf);

}

int dc_parse_command(int i_argc, char ** p_argv, CmdData * p_cmdd) {

	int i;

	char * psz_command_usage =
			"Usage: dashcast [options]\n"
					"\n"
					"Options:\n"
					"\n"
					"    -a a_source                  audio source\n"
					"    -v v_source                  video source\n"
					"    -av av_source                multiplexed audio and video source\n"
					"    -vf v_format                 video format (if necessary)\n"
					"    -vfr v_framerate             video framerate (if necessary)\n"
					"    -af a_format                 audio format (if necessary)\n"
					"    -live                        indicates that the system is live\n"
					"    -conf conf_file              configuration file [default=dashcast.conf]\n"
					"    -seg-dur seg_duration        segment duration [default=1000]\n"
					"    -frag-dur frag_duration      fragment duration [default=1000]\n"
					"    -mpd mpd_file                MPD file name [default=dashcast.mpd]\n"
					"    -out out_dir                 output data directory name [default=output]\n"
					"\n";

	char * psz_command_error =
			"\33[31mUnknown option or missing mandatory argument.\33[0m\n";

	if (i_argc == 1) {
		printf("%s", psz_command_usage);
		return -1;
	}

	i = 1;
	while (i < i_argc) {

		if (strcmp(p_argv[i], "-a") == 0 || strcmp(p_argv[i], "-v") == 0
				|| strcmp(p_argv[i], "-av") == 0) {

			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (strcmp(p_argv[i - 1], "-a") == 0
					|| strcmp(p_argv[i - 1], "-av") == 0) {
				if (strcmp(p_cmdd->adata.psz_name, "") != 0) {
					printf("Audio source has been already specified.\n");
					printf("%s", psz_command_usage);
					return -1;
				}
				//p_cmdd->psz_asrc = malloc(strlen(p_argv[i]) + 1);
				strcpy(p_cmdd->adata.psz_name, p_argv[i]);
				strcat(p_cmdd->adata.psz_name, "\0");
			}

			if (strcmp(p_argv[i - 1], "-v") == 0
					|| strcmp(p_argv[i - 1], "-av") == 0) {
				if (strcmp(p_cmdd->vdata.psz_name, "") != 0) {
					printf("Video source has been already specified.\n");
					printf("%s", psz_command_usage);
					return -1;
				}

				//p_cmdd->psz_vsrc = malloc(strlen(p_argv[i]) + 1);
				strcpy(p_cmdd->vdata.psz_name, p_argv[i]);
				strcat(p_cmdd->vdata.psz_name, "\0");
			}

			i++;

		} else if (strcmp(p_argv[i], "-af") == 0
				|| strcmp(p_argv[i], "-vf") == 0) {

			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (strcmp(p_argv[i - 1], "-af") == 0) {
				if (strcmp(p_cmdd->adata.psz_format, "") != 0) {
					printf("Audio format has been already specified.\n");
					printf("%s", psz_command_usage);
					return -1;
				}
				//p_cmdd->psz_af = malloc(strlen(p_argv[i]) + 1);
				strcpy(p_cmdd->adata.psz_format, p_argv[i]);
				strcat(p_cmdd->adata.psz_format, "\0");
			}

			if (strcmp(p_argv[i - 1], "-vf") == 0) {
				if (strcmp(p_cmdd->vdata.psz_format, "") != 0) {
					printf("Video format has been already specified.\n");
					printf("%s", psz_command_usage);
					return -1;
				}

				//p_cmdd->psz_vf = malloc(strlen(p_argv[i]) + 1);
				strcpy(p_cmdd->vdata.psz_format, p_argv[i]);
				strcat(p_cmdd->vdata.psz_format, "\0");
			}

			i++;

		} else if (strcmp(p_argv[i], "-vfr") == 0) {

			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (p_cmdd->vdata.i_framerate != -1) {
				printf("Video framerate has been already specified.\n");
				printf("%s", psz_command_usage);
				return -1;
			}
			//p_cmdd->psz_vfr = malloc(strlen(p_argv[i]) + 1);
			p_cmdd->vdata.i_framerate = atoi(p_argv[i]);
			//strcpy(p_cmdd->psz_vfr, p_argv[i]);
			//strcat(p_cmdd->psz_vfr, "\0");

			i++;

		} else if (strcmp(p_argv[i], "-conf") == 0) {
			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			p_cmdd->p_conf = gf_cfg_force_new(NULL, p_argv[i]);

			i++;

		} else if (strcmp(p_argv[i], "-out") == 0) {
			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			strcpy(p_cmdd->psz_out, p_argv[i]);
			strcat(p_cmdd->psz_out, "\0");

			i++;

		} else if (strcmp(p_argv[i], "-mpd") == 0) {
			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (strcmp(p_cmdd->psz_mpd, "") != 0) {
				printf("MPD file has been already specified.\n");
				printf("%s", psz_command_usage);
				return -1;
			}

			strncpy(p_cmdd->psz_mpd, p_argv[i], GF_MAX_PATH);
			i++;

		} else if (strcmp(p_argv[i], "-seg-dur") == 0) {
			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (p_cmdd->i_seg_dur != 0) {
				printf("Segment duration has been already specified.\n");
				printf("%s", psz_command_usage);
				return -1;
			}

			p_cmdd->i_seg_dur = atoi(p_argv[i]);
			i++;

		} else if (strcmp(p_argv[i], "-frag-dur") == 0) {
			i++;
			if (i >= i_argc) {
				printf("%s", psz_command_error);
				printf("%s", psz_command_usage);
				return -1;
			}

			if (p_cmdd->i_frag_dur != 0) {
				printf("Fragment duration has been already specified.\n");
				printf("%s", psz_command_usage);
				return -1;
			}

			p_cmdd->i_frag_dur = atoi(p_argv[i]);
			i++;

		} else if (strcmp(p_argv[i], "-live") == 0) {
			p_cmdd->i_live = 1;
			i++;
		} else {
			printf("%s", psz_command_error);
			printf("%s", psz_command_usage);
			return -1;
		}

	}

	if (strcmp(p_cmdd->psz_mpd, "") == 0) {
		strcpy(p_cmdd->psz_mpd, "dashcast.mpd");
	}

	if (strcmp(p_cmdd->psz_out, "") == 0) {
		strcpy(p_cmdd->psz_out, "output/");

		struct stat status;

		if (stat(p_cmdd->psz_out, &status) != 0) {
			mkdir(p_cmdd->psz_out, 0777);
		}
	}

	if (strcmp(p_cmdd->vdata.psz_name, "") == 0
			&& strcmp(p_cmdd->adata.psz_name, "") == 0) {
		printf("Audio/Video source must be specified.\n");
		printf("%s", psz_command_usage);
		return -1;
	}

	if(p_cmdd->i_seg_dur == 0) {
		p_cmdd->i_seg_dur = 1000;
	}

	if(p_cmdd->i_frag_dur == 0) {
		p_cmdd->i_frag_dur = p_cmdd->i_seg_dur;
	}

	printf("\33[34m\33[1m");
	printf("Options:\n");
	printf("    video source: %s\n", p_cmdd->vdata.psz_name);
	if (strcmp(p_cmdd->vdata.psz_format, "") != 0) {
		printf("    video format: %s\n", p_cmdd->vdata.psz_format);
	}
	if (p_cmdd->vdata.i_framerate != -1) {
		printf("    video framerate: %d\n", p_cmdd->vdata.i_framerate);
	}

	printf("    audio source: %s\n", p_cmdd->adata.psz_name);
	if (strcmp(p_cmdd->adata.psz_format, "") != 0) {
		printf("    audio format: %s\n", p_cmdd->adata.psz_format);
	}
	printf("\33[0m");
	fflush(stdout);

	if (!p_cmdd->p_conf) {
		p_cmdd->p_conf = gf_cfg_force_new(NULL, "dashcast.conf");
	}

	dc_read_configuration(p_cmdd);

	return 0;
}

