// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  minidir.c - Minimal core directory access functions
//
//============================================================

#include "emu.h"
#include "osdcore.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>



struct osd_directory
{
	DIR *fd;
	char path[256];
	osd_directory_entry ent;
};


//============================================================
//  osd_opendir
//============================================================


osd_directory *osd_opendir(const char *dirname)
{

	osd_directory *dir = (osd_directory*)osd_malloc(sizeof(osd_directory));
	if(dir==NULL)
		return NULL;

	dir->fd = opendir(dirname);
	if(dir->fd==NULL){
		printf("osd_opendir(%s) failed!\n", dirname);
		return NULL;
	}

	strncpy(dir->path, dirname, 256);

	return dir;
}


//============================================================
//  osd_readdir
//============================================================

const osd_directory_entry *osd_readdir(osd_directory *dir)
{
	int retv;
	char full_path[256];
	struct dirent *dt;
	struct stat st;

	dt = readdir(dir->fd);
	if(dt==NULL){
		return NULL;
	}

	dir->ent.name = dt->d_name;

	sprintf(full_path, "%s/%s", dir->path, dt->d_name);
	retv = stat(full_path, &st);
	if(retv<0){
		printf("osd_readdir: can stat %s!\n", full_path);
		return NULL;
	}

	if(S_ISDIR(st.st_mode)){
		dir->ent.type = ENTTYPE_DIR;
	}else{
		dir->ent.type = ENTTYPE_FILE;
	}

	dir->ent.size = st.st_size;

	return &dir->ent;
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir)
{
	if (dir->fd != NULL)
		closedir(dir->fd);
	osd_free(dir);
}


//============================================================
//  osd_is_absolute_path
//============================================================

int osd_is_absolute_path(const char *path)
{
	if( (path[0] == '/') || (path[0] == '\\') )
		return TRUE;
	else
		return FALSE;
}

