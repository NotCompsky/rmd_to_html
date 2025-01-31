#include "md_to_html.h"

#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <string>
#include <compsky/os/write.hpp> // for write
#include <algorithm>

extern const char* blockquote_tagname;
bool PRINT_DEBUG = false;
bool IS_VERBOSE = false;
bool INCLUDE_COMMENT_NODES = false;
extern std::vector<Filename> replacewith_filenames;

int main(int argc,  const char* const* argv){
	bool any_errors = false;
	++argv;
	--argc;
	if (argc != 0){
	while((argv[0][0] == '-') and (argv[0][2] == 0)){
		switch(argv[0][1]){
			case 'b':
				blockquote_tagname = *(++argv);
				--argc;
				break;
			case 'c':
				INCLUDE_COMMENT_NODES = true;
				break;
			case 'd':
				PRINT_DEBUG = true;
				break;
			case 'v':
				IS_VERBOSE = true;
				break;
			case 'R': {
				const char* const dirpath = *(++argv);
				--argc;
				std::size_t dirpath_len = strlen(dirpath);
				char fullpath[4096];
				memcpy(fullpath, dirpath, dirpath_len);
				if (fullpath[dirpath_len-1] != '/'){
					fullpath[dirpath_len] = '/';
					++dirpath_len;
				}
				DIR* const dir = opendir(dirpath);
				if (unlikely(dir == nullptr)){
					any_errors = true;
				} else {
					struct dirent* ent;
					while((ent = readdir(dir))){
						if (likely((ent->d_type == DT_REG) or (ent->d_type == DT_LNK))){
							replacewith_filenames.emplace_back(fullpath, dirpath_len, ent->d_name);
						}
					}
					std::sort(replacewith_filenames.begin(), replacewith_filenames.end(), [](const Filename& f1,  const Filename& f2){
						return f1.name.size() > f2.name.size();
					}); // Place largest names first - so R_E_P_L_A_C_E_xy comes before R_E_P_L_A_C_E_x
				}
				break;
			}
			default:
				any_errors = true;
				break;
		}
		++argv;
		--argc;
	}
	if (likely(not any_errors) and likely((argc >= 1) and (argc <= 2))){
		int out_fd = 1;
		if (argc == 2){
			out_fd = open(argv[1], O_WRONLY|O_CREAT, 0644);
		}
		char* const html_buf = reinterpret_cast<char*>(malloc(2*HALF_BUF_SZ));
		if (likely(html_buf != nullptr)){
			char* const html_end = md_to_html(argv[0], html_buf);
			write(out_fd, html_buf, compsky::utils::ptrdiff(html_end,html_buf));
			if (out_fd != 1)
				close(out_fd);
			for (const Filename& filename : replacewith_filenames){
				filename.deconstruct();
			}
			return 0;
		}
	}
	}
	constexpr const char* errmsg =
		"USAGE: [[OPTIONS]] [/path/to/file.rmd] [/path/to/outfile.html]?\n"
		"OPTIONS:\n"
		"	-b BLOCKQUOTE_TAGNAME\n"
		"		Default is \"blockquote\"\n"
		"	-d\n"
		"		Debug\n"
		"	-v\n"
		"		Verbose\n"
		"	-c\n"
		"		Include <!-- comment nodes --> (default FALSE)\n"
		"	-R [/path/to/directory]\n"
		"		Directory containing files.\n"
		"		For each file named {fname}, if a string \"R_E_P_L_A_C_E_{fname}\" is encountered, it is replaced by the file's contents.\n"
		"		Can screw up things if done in strange places. <R_E_P_L_A_C_E_stuff> works but not <blahR_E_P_L_A_C_E_stuff>\n"
	;
	write(2, errmsg, std::char_traits<char>::length(errmsg));
	return 1;
}
