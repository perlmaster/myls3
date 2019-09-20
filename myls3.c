/*********************************************************************
*
* File      : myls3.c
*
* Author    : Barry Kimelman
*
* Created   : September 18, 2019
*
* Purpose   : List files in a directcory (similar to ls).
*
*********************************************************************/

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<string.h>
#include	<time.h>
#include	<string.h>
#include	<stdarg.h>
#include	<getopt.h>

#define	EQ(s1,s2)	(strcmp(s1,s2)==0)
#define	NE(s1,s2)	(strcmp(s1,s2)!=0)
#define	GT(s1,s2)	(strcmp(s1,s2)>0)
#define	LT(s1,s2)	(strcmp(s1,s2)<0)
#define	LE(s1,s2)	(strcmp(s1,s2)<=0)

typedef	struct filedata_tag {
	struct filedata_tag	*next;
	char	*filename;
	struct _stat	filestats;
} FILEDATA;

typedef	struct list_tag {
	int			count;
	FILEDATA	*first;
	FILEDATA	*last;
} LIST;

typedef struct name_tag {
	char	*name;
	struct name_tag	*next_name;
} NAME;
typedef struct nameslist_tag {
	NAME	*first_name;
	NAME	*last_name;
	int		num_names;
} NAMESLIST;

static	char	ftypes[] = {
 '.' , 'p' , 'c' , '?' , 'd' , '?' , 'b' , '?' , '-' , '?' , 'l' , '?' , 's' , '?' , '?' , '?'
};

static char	*perms[] = {
	"---" , "--x" , "-w-" , "-wx" , "r--" , "r-x" , "rw-" , "rwx"
};

static char	*months[12] = { "Jan" , "Feb" , "Mar" , "Apr" , "May" , "Jun" ,
				"Jul" , "Aug" , "Sep" , "Oct" , "Nov" , "Dec" } ;

static	int		opt_d = 0 , opt_t = 0 , opt_s = 0 , opt_R = 0;
static	int		opt_n = 0 , opt_D = 0 , opt_r = 0 , opt_h = 0;
static	int		num_args;
static	LIST	Files = { 0 , NULL , NULL };

extern	int		optind , optopt , opterr;

extern	void	system_error() , quit() , die();

/*********************************************************************
*
* Function  : debug_print
*
* Purpose   : Display an optional debugging message.
*
* Inputs    : char *format - the format string (ala printf)
*             ... - the data values for the format string
*
* Output    : the debugging message
*
* Returns   : nothing
*
* Example   : debug_print("The answer is %s\n",answer);
*
* Notes     : (none)
*
*********************************************************************/

void debug_print(char *format,...)
{
	va_list ap;

	if ( opt_D ) {
		va_start(ap,format);
		vfprintf(stdout, format, ap);
		fflush(stdout);
		va_end(ap);
	} /* IF debug mode is on */

	return;
} /* end of debug_print */

/*********************************************************************
*
* Function  : usage
*
* Purpose   : Display a program usage message
*
* Inputs    : char *pgm - name of program
*
* Output    : the usage message
*
* Returns   : nothing
*
* Example   : usage("The answer is %s\n",answer);
*
* Notes     : (none)
*
*********************************************************************/

void usage(char *pgm)
{
	fprintf(stderr,"Usage : %s [-hFgiDdtsnr]\n\n",pgm);
	fprintf(stderr,"D - invoke debugging mode\n");
	fprintf(stderr,"d - only list the dirname, not its contents\n");
	fprintf(stderr,"t - sort filenames by time\n");
	fprintf(stderr,"s - sort filenames by size\n");
	fprintf(stderr,"n - sort filenames by name\n");
	fprintf(stderr,"r - reverse sort order\n");
	fprintf(stderr,"h - produce this summary\n");
	fprintf(stderr,"R - recursively process directories\n");

	return;
} /* end of usage */

/*********************************************************************
*
* Function  : trim_trailing_chars
*
* Purpose   : Trim occurrences of the specified char from the end of
*             the specified buffer
*
* Inputs    : char *in_buffer - the buffer to be trimmed
*             char trim_ch - the char to be trimmed
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : trim_trailing_chars(buffer,'/');
*
* Notes     : (none)
*
*********************************************************************/

void trim_trailing_chars(char *in_buffer, char trim_ch)
{
	char	*ptr , ch;

	ptr = &in_buffer[strlen(in_buffer)-1];
	for ( ch = *ptr ; ch == trim_ch && ptr > in_buffer ; ch = *--ptr ) {
		*ptr = '\0';
	} /* FOR */

	return;
} /* end of trim_trailing_chars */

/*********************************************************************
*
* Function  : dump_list
*
* Purpose   : Dump list.
*
* Inputs    : char *title - optional title
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : dump_list("List...\n");
*
* Notes     : (none)
*
*********************************************************************/

void dump_list(char *title)
{
	FILEDATA	*node;

	if ( title != NULL ) {
		printf("%s",title);
	} /* IF */
	node = Files.first;
	for ( ; node != NULL ; node = node->next ) {
		printf(">> %s\n",node->filename);
	} /* FOR */
	printf("\n");
	fflush(stdout);

	return;
} /* end of dump_list */

/*********************************************************************
*
* Function  : append_file_to_list
*
* Purpose   : Append a new entry to the list of files.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = append_file_to_list(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *append_file_to_list(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node;

	file_node = (FILEDATA *)calloc(1,sizeof(FILEDATA));
	if ( file_node == NULL ) {
		quit(1,"calloc failed");
	} /* IF */
	file_node->filename = _strdup(filename);
	if ( file_node->filename == NULL ) {
		quit(1,"strdup failed");
	} /* IF */
	memcpy(&file_node->filestats,filestats,sizeof(struct _stat));

	if ( Files.count == 0 ) {
		Files.first = file_node;
	} /* IF */
	else {
		Files.last->next = file_node;
	} /* ELSE */
	Files.last = file_node;
	Files.count += 1;

	return(file_node);
} /* end of append_file_to_list */

/*********************************************************************
*
* Function  : prepend_file_to_list
*
* Purpose   : Prepend a new entry to the list of files.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = prepend_file_to_list(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *prepend_file_to_list(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node;

	file_node = (FILEDATA *)calloc(1,sizeof(FILEDATA));
	if ( file_node == NULL ) {
		quit(1,"calloc failed");
	} /* IF */
	file_node->filename = _strdup(filename);
	if ( file_node->filename == NULL ) {
		quit(1,"strdup failed");
	} /* IF */
	memcpy(&file_node->filestats,filestats,sizeof(struct _stat));

	if ( Files.count == 0 ) {
		Files.last = file_node;
	} /* IF */
	else {
		file_node->next = Files.first;
	} /* ELSE */
	Files.first = file_node;
	Files.count += 1;

	return(file_node);
} /* end of prepend_file_to_list */

/*********************************************************************
*
* Function  : add_to_list_by_name
*
* Purpose   : Add a new entry to the list of files based on name.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = add_to_list_by_name(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *add_to_list_by_name(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node , *prev , *curr;

	if ( Files.count <= 0 || GT(filename,Files.last->filename) ) {
		file_node = append_file_to_list(filename,filestats);
	} /* IF */
	else {
		if ( LE(filename,Files.first->filename) ) {
			file_node = prepend_file_to_list(filename,filestats);
		} /* IF */
		else {
			prev = NULL;
			curr = Files.first;
			for ( ; GT(filename,curr->filename) ; curr = curr->next ) {
				prev = curr;
			} /* FOR */
			file_node = (FILEDATA *)calloc(1,sizeof(FILEDATA));
			if ( file_node == NULL ) {
				quit(1,"calloc failed");
			} /* IF */
			file_node->filename = _strdup(filename);
			if ( file_node->filename == NULL ) {
				quit(1,"strdup failed");
			} /* IF */
			memcpy(&file_node->filestats,filestats,sizeof(struct _stat));
			file_node->next = curr;
			prev->next = file_node;
			Files.count += 1;
		} /* ELSE */
	} /* ELSE */

	return(file_node);
} /* end of add_to_list_by_name */

/*********************************************************************
*
* Function  : add_to_list_by_size
*
* Purpose   : Add a new entry to the list of files based on size.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = add_to_list_by_size(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *add_to_list_by_size(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node , *prev , *curr;

	if ( Files.count <= 0 || filestats->st_size > Files.last->filestats.st_size ) {
		file_node = append_file_to_list(filename,filestats);
	} /* IF */
	else {
		if ( filestats->st_size <= Files.first->filestats.st_size ) {
			file_node = prepend_file_to_list(filename,filestats);
		} /* IF */
		else {
			prev = NULL;
			curr = Files.first;
			for ( ; filestats->st_size > curr->filestats.st_size ; curr = curr->next ) {
				prev = curr;
			} /* FOR */
			file_node = (FILEDATA *)calloc(1,sizeof(FILEDATA));
			if ( file_node == NULL ) {
				quit(1,"calloc failed");
			} /* IF */
			file_node->filename = _strdup(filename);
			if ( file_node->filename == NULL ) {
				quit(1,"strdup failed");
			} /* IF */
			memcpy(&file_node->filestats,filestats,sizeof(struct _stat));
			file_node->next = curr;
			prev->next = file_node;
			Files.count += 1;
		} /* ELSE */
	} /* ELSE */

	return(file_node);
} /* end of add_to_list_by_size */

/*********************************************************************
*
* Function  : add_to_list_by_time
*
* Purpose   : Add a new entry to the list of files based on time.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = add_to_list_by_time(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *add_to_list_by_time(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node , *prev , *curr;

	if ( Files.count <= 0 || filestats->st_mtime > Files.last->filestats.st_mtime ) {
		file_node = append_file_to_list(filename,filestats);
	} /* IF */
	else {
		if ( filestats->st_mtime <= Files.first->filestats.st_mtime ) {
			file_node = prepend_file_to_list(filename,filestats);
		} /* IF */
		else {
			prev = NULL;
			curr = Files.first;
			for ( ; filestats->st_mtime > curr->filestats.st_mtime ; curr = curr->next ) {
				prev = curr;
			} /* FOR */
			file_node = (FILEDATA *)calloc(1,sizeof(FILEDATA));
			if ( file_node == NULL ) {
				quit(1,"calloc failed");
			} /* IF */
			file_node->filename = _strdup(filename);
			if ( file_node->filename == NULL ) {
				quit(1,"strdup failed");
			} /* IF */
			memcpy(&file_node->filestats,filestats,sizeof(struct _stat));
			file_node->next = curr;
			prev->next = file_node;
			Files.count += 1;
		} /* ELSE */
	} /* ELSE */

	return(file_node);
} /* end of add_to_list_by_time */

/*********************************************************************
*
* Function  : add_file_to_list
*
* Purpose   : Add a new entry to the list of files.
*
* Inputs    : char *filename - name of file
*             struct _stat *filestats - ptr to stat structure
*
* Output    : (none)
*
* Returns   : FILEDATA *node - ptr to newly created list entry
*
* Example   : node = add_file_to_list(filename,&filestats);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *add_file_to_list(char *filename, struct _stat *filestats)
{
	FILEDATA	*file_node;

	debug_print("add_file_to_list(%s)\n",filename);
	if ( opt_n ) {
		file_node = append_file_to_list(filename,filestats);
	} else if ( opt_t ) {
		file_node = add_to_list_by_time(filename,filestats);
	} else if (opt_s ) {
		file_node = add_to_list_by_size(filename,filestats);
	} else {
		file_node = add_to_list_by_name(filename,filestats);
	} /* ELSE */

	return(file_node);
} /* end of add_file_to_list */

/*********************************************************************
*
* Function  : list_directory
*
* Purpose   : List the files under a directory.
*
* Inputs    : char *dirname - name of directory
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : list_directory(dirname);
*
* Notes     : (none)
*
*********************************************************************/

int list_directory(char *dirpath)
{
	_DIR	*dirptr;
	struct _dirent	*entry;
	struct _stat	filestats;
	char	*name , filename[1024] , dirname[1024];
	int		current_directory;
	NAMESLIST	subdirs;
	unsigned short	filemode;
	NAME	*dir;

	debug_print("list_directory(%s)\n",dirpath);

	subdirs.num_names = 0;
	subdirs.first_name = NULL;
	subdirs.last_name = NULL;

	strcpy(dirname,dirpath);
	trim_trailing_chars(dirname,'/');
	dirptr = _opendir(dirname);
	if ( dirptr == NULL ) {
		quit(1,"_opendir failed for \"%s\"",dirname);
	}

	current_directory = EQ(dirname,".");
	entry = _readdir(dirptr);
	for ( ; entry != NULL ; entry = _readdir(dirptr) ) {
		name = entry->d_name;
		if ( current_directory )
			strcpy(filename,name);
		else
			sprintf(filename,"%s/%s",dirname,name);
		if ( _stat(filename,&filestats) < 0 ) {
			system_error("stat() failed for \"%s\"",filename);
		} /* IF */
		else {
			add_file_to_list(filename,&filestats);
			filemode = filestats.st_mode & _S_IFMT;
			if ( _S_ISDIR(filemode) && opt_R && NE(name,".") && NE(name,"..") ) {
				subdirs.num_names += 1;
				dir = (NAME *)calloc(1,sizeof(NAME));
				if ( dir == NULL ) {
					quit(1,"calloc failed for NAME");
				}
				dir->name = _strdup(filename);
				if ( dir->name == NULL ) {
					quit(1,"strdup failed for dir.name");
				}
				if ( subdirs.num_names == 1 ) {
					subdirs.first_name = dir;
				}
				else {
					subdirs.last_name->next_name = dir;
				}
				subdirs.last_name = dir;
			} /* IF recursive processing requested */
		} /* ELSE */
	} /* FOR */
	debug_print("list_directory(%s) ; all entries processed\n",dirname);
	_closedir(dirptr);

	if ( opt_R ) {
		dir = subdirs.first_name;
		for ( ; dir != NULL ; dir = dir->next_name ) {
			debug_print("list_directory() : recursively process '%s' under '%s'\n",dir->name,dirpath);
			list_directory(dir->name);
		}
	}

	return(0);
} /* end of list_directory */

/*********************************************************************
*
* Function  : Reverse
*
* Purpose   : Reverse the order of the elements in a list
*
* Inputs    : FILEDATA *curr - pointer to the top of the list
*
* Output    : (none)
*
* Returns   : pointer to the new top of the list
*
* Example   : new_first = Reverse(first);
*
* Notes     : (none)
*
*********************************************************************/

FILEDATA *Reverse (FILEDATA *curr)
{
	FILEDATA *prev = NULL;

	while (curr != NULL)
	{
		FILEDATA *tmp = curr->next;

		curr->next = prev;
		prev = curr;
		curr = tmp;
	}
	return prev;
}

/*********************************************************************
*
* Function  : format_mode
*
* Purpose   : Format binary permission bits into a printable ASCII string
*
* Inputs    : unsigned short file_mode - mode bits from stat()
*             char *mode_bits - buffer to receive formatted info
*
* Output    : (none)
*
* Returns   : formatted mode info
*
* Example   : format_mode(filestat->st_mode,mode_info);
*
* Notes     : (none)
*
*********************************************************************/

void format_mode(unsigned short file_mode, char *mode_info)
{
	unsigned short setids;
	char *permstrs[3] , ftype , *ptr;

	setids = (file_mode & 07000) >> 9;
	permstrs[0] = perms[ (file_mode & 0700) >> 6 ];
	permstrs[1] = perms[ (file_mode & 0070) >> 3 ];
	permstrs[2] = perms[ file_mode & 0007 ];
	ftype = ftypes[ (file_mode & 0170000) >> 12 ];
	if ( setids ) {
		if ( setids & 01 ) { // sticky bit
			ptr = permstrs[2];
			if ( ptr[2] == 'x' ) {
				ptr[2] = 't';
			}
			else {
				ptr[2] = 'T';
			}
		}
		if ( setids & 04 ) { // setuid bit
			ptr = permstrs[0];
			if ( ptr[2] == 'x' ) {
				ptr[2] = 's';
			}
			else {
				ptr[2] = 'S';
			}
		}
		if ( setids & 02 ) { // setgid bit
			ptr = permstrs[1];
			if ( ptr[2] == 'x' ) {
				ptr[2] = 's';
			}
			else {
				ptr[2] = 'S';
			}
		}
	} // IF setids
	sprintf(mode_info,"%c%3.3s%3.3s%3.3s",ftype,permstrs[0],permstrs[1],permstrs[2]);

	return;
} /* end of format_mode */

/*********************************************************************
*
* Function  : display_file_info
*
* Purpose   : Display information for one file
*
* Inputs    : char *filename - name of file
*
* Output    : file information
*
* Returns   : nothing
*
* Example   : display_file_info("foo.txt");
*
* Notes     : (none)
*
*********************************************************************/

void display_file_info(char *filepath)
{
	struct _stat	filestats;
	unsigned short	filemode;
	char	mode_info[1024] , file_date[256];
	struct tm	*filetime;

	if ( _stat(filepath,&filestats) == 0 ) {
		filemode = filestats.st_mode & _S_IFMT;
		format_mode(filestats.st_mode,mode_info);
		filetime = localtime(&filestats.st_mtime);
		sprintf(file_date,"%3.3s %2d, %d %02d:%02d:%02d",
			months[filetime->tm_mon],
			filetime->tm_mday,1900+filetime->tm_year,filetime->tm_hour,filetime->tm_min,
			filetime->tm_sec);
		printf("%s %4d %10d %s %s\n",mode_info,filestats.st_nlink,filestats.st_size,file_date,filepath);
	}

	return;
} /* end of display_file_info */

/*********************************************************************
*
* Function  : main
*
* Purpose   : program entry point
*
* Inputs    : argc - number of parameters
*             argv - list of parameters
*
* Output    : (none)
*
* Returns   : (nothing)
*
* Example   : myls *.h
*
* Notes     : (none)
*
*********************************************************************/

int main(int argc, char *argv[])
{
	int		errflag , c;
	char	*filename;
	struct _stat	filestats;
	unsigned short	filemode;
	FILEDATA	*file_node;

	errflag = 0;
	while ( (c = _getopt(argc,argv,":hgiDdtsnrR")) != -1 ) {
		switch (c) {
		case 'h':
			opt_h = 1;
			break;
		case 'r':
			opt_r = 1;
			break;
		case 'R':
			opt_R = 1;
			break;
		case 'd':
			opt_d = 1;
			break;
		case 'D':
			opt_D = 1;
			break;
		case 't':
			opt_t = 1;
			break;
		case 's':
			opt_s = 1;
			break;
		case 'n':
			opt_n = 1;
			break;
		case '?':
			printf("Unknown option '%c'\n",optopt);
			errflag += 1;
			break;
		case ':':
			printf("Missing value for option '%c'\n",optopt);
			errflag += 1;
			break;
		default:
			printf("Unexpected value from getopt() '%c'\n",c);
		} /* SWITCH */
	} /* WHILE */
	if ( errflag ) {
		usage(argv[0]);
		die(1,"\nAborted due to parameter errors\n");
	} /* IF */
	if ( opt_t + opt_s + opt_n  > 1 ) {
		die(1,"Only one of 't' , 's' and 'n' can be specified\n");
	} /* IF */
	if ( opt_h ) {
		usage(argv[0]);
		exit(0);
	} /* IF */

	num_args = argc - optind;
	if ( num_args <= 0 ) {
		list_directory(".");
	} /* IF */
	else {
		filename = argv[optind];
		for ( ; optind < argc ; filename = argv[++optind] ) {
			if ( _stat(filename,&filestats) < 0 ) {
				system_error("stat() failed for \"%s\"",filename);
			} /* IF */
			else {
				filemode = filestats.st_mode & _S_IFMT;
				if ( _S_ISDIR(filemode) && opt_d == 0 ) {
					list_directory(filename);
				} /* IF */
				else {
					add_file_to_list(filename,&filestats);
				} /* ELSE */
			} /* ELSE */
		} /* FOR */
	} /* ELSE */

	debug_print("Check for reversal\n");
	if ( opt_r ) {
		file_node = Reverse(Files.first);
	} /* IF */
	else {
		file_node = Files.first;
	} /* ELSE */

	debug_print("\nList info for files\n");
	for ( ; file_node != NULL ; file_node = file_node->next ) {
		display_file_info(file_node->filename);
	} /* FOR */

	exit(0);
} /* end of main */
