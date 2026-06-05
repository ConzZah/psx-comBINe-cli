/*******************************************************************************
*
* NOTE: THIS IS A FORK BY ConzZah, IT GETS RID OF ALL GUI ELEMENTS,
* BECAUSE I DESPISE WXWIDGETS, IT'S A PAIN IN THE AAH
*
* This file is part of psx-comBINe.
* Please see the github: https://github.com/ADBeta/psx-comBINe
* This and all other files relating to psx-conBINe are under the GPLv2.0 License
*
* psx-comBINe is a simple program to combine multiple .bin files into a single
* file, and modified the .cue file indexing
*
* ADBeta (c)	Ver 5.2.2    18 Dec 2024
*
* ConzZah (c) (CLI FORK) 	 2026-06-05
*
*******************************************************************************/
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>

#include "cuehandler.hpp"
#include "clampp.hpp"
#include "utils.hpp"

/*** Globals *****************************************************************/
//Define how large the RAM byte array while dumping should be. (4KB)
#define _BINARY_ARRAY_SIZE 4096

namespace message {
const char *copyright = "\npsx-comBINe-cli v5.2.2 (fork) ORIGINAL CODE BY: ADBeta(c)\n\n(forked on 2026-06-05 by ConzZah to get rid of wxwidgets)\n";

const char *short_help =
"Usage: psx-combine-cli [input.cue or directory] [options]\n\
use --help for full help information\n";

const char *long_help =
"\nUsage: psx-combine-cli [input.cue or directory] [options]\n\n\
By default psx-comBINe takes a single input.cue file, creates a directory in\n\
the .cue's parent directory called \"psx-comBINe\".\n\
it will then output the combined .bin and .cue file, leaving the original \n\
files untouched.\n\n\
Options:\n\
-h, --help\t\tShow this help message\n\
-v, --verbose\t\tPrint a verbose CUE sheet diagnostics before dumping\n\
-d, --directory\t\tChange the output directory\n\
\t\t\tpsx-combine ./input.cue -d /home/user/games\n\n\
-f, --filename\t\tSpecify the output .cue filename\n\
\t\t\tpsx-combine ./input.cue -f combined_game.cue (or combined_game)\n\n";

//Messages for throw()
const char *missing_filepath = "Filename or directory was not specified";
const char *invalid_filepath = "Input path is invalid, or does not exist";
const char *filepath_bad_extension = "Input file must be a .cue file";
const char *dir_missing_cue = "Input directory does not contain any .cue files";
const char *cue_has_no_files = "Input .cue file has no FILEs";

const char *filename_has_dir = "filename argument must only contain a filename";
const char *filename_bad_extension = "filename extension must be .cue";

const char *input_bin_not_open = "The input file could not be opened";
const char *output_bin_create_failed = "Output binary file could not be created";

} //namespace message


/*** Enums & Classes **********************************************************/
// clampp Argument Indexes
struct ClamppArguments {
	int help_idx;	   // Help flag
	int dir_idx;		// Directory flag
	int file_idx;	   // File flag
	int verbose_idx;	// Verbose flag
};

// System control variables, Set via CLI events
struct SystemVariables {
	FilesystemType input_fstype, output_fstype;			// Filsesystem type
	std::filesystem::path input_dir_path, output_dir_path; // Directory paths
	std::filesystem::path input_cue_path, output_cue_path; // Cue Paths
	std::filesystem::path input_bin_path, output_bin_path; // Binary Paths
	CueSheet input_cue_sheet, output_cue_sheet;			// Cue Sheet Objects

	bool verbose;
};


/***  Forward Declarations *****************************************************/
/// @breif Gets the System Variables from CLI Arguements
/// @param &clampp_args, input CLI args to parse
/// @param &system_vars, system vars to set from CLI input
/// @return none
void CLIGetVars(ClamppClass &cli_handler, ClamppArguments &cli_args,
				SystemVariables &system_vars);

/// @breif Combines the fields in the input cue into one, for the output file
/// @param &system_vars System Variables from CLI
/// @return none
void CombineCue(SystemVariables &system_vars);

/// @breiif Goes through the input .cue file, combining all .bin files within
/// into a single output .bin file
/// @param &system_vars System Variables from CLI
/// @return status string for CLI printing
std::string DumpBinaryFiles(SystemVariables &system_vars);


// Define a global system variables struct
SystemVariables sys_vars;

/*** Main *********************************************************************/

int main(const int argc, const char *argv[]) {
	// clampp Argument handler object
	ClamppClass cli_handler;

	// Clampp Argument struct
	ClamppArguments cli_args;
	cli_args.help_idx	= cli_handler.AddDefinition("--help", "-h", false);
	cli_args.dir_idx	 = cli_handler.AddDefinition("--directory", "-d", true);
	cli_args.file_idx	= cli_handler.AddDefinition("--filename", "-f", true);
	cli_args.verbose_idx = cli_handler.AddDefinition("--verbose", "-v", false);


	/** User Argument handling ************************************************/
	// Scan the arguments
	ClamppConfig::allow_undefined_args = true;
	int scan_ret = cli_handler.ScanArgs(argc - 1, argv + 1);

	// If no substring was given when expected, error
	if(scan_ret == CLAMPP_ENOSUBSTR) {
		std::cerr << "Argument is missing a parameter. " << message::short_help
				  << std::endl;
		exit(EXIT_FAILURE);
	}

	// If help was requested, print the long help message then exit.
	if(cli_handler.GetDetectedStatus(cli_args.help_idx)) {
		std::cout << message::long_help << message::copyright << std::endl;
		exit(EXIT_SUCCESS);
	}

	// CLI Mode
	{
		// Get the CLI Arguments
		CLIGetVars(cli_handler, cli_args, sys_vars);
		// Combine the .cue file variables
		CombineCue(sys_vars);
		// Dump the .cue binary files into one output file
		std::string status = DumpBinaryFiles(sys_vars);
		std::cout << "\n" << status << std::endl;
	}

	return 0;
}


/*** Util Functions **********************************************************/
void CLIGetVars(ClamppClass &cli_handler, ClamppArguments &cli_args,
				SystemVariables &system_vars) {
	system_vars.verbose = cli_handler.GetDetectedStatus(cli_args.verbose_idx);

	// Get Filesystem variables
	try {
		// Make sure a filepath string has been passed
		std::filesystem::path arg_filepath(cli_handler.GetUndefinedArg(0));
		if(arg_filepath.empty()) {
			throw std::invalid_argument(message::missing_filepath);
		}

		// Check the type of the arg_path, and guard an invalid input
		system_vars.input_fstype = GetPathType(arg_filepath);
		if(system_vars.input_fstype == FilesystemType::Invalid) {
			throw std::invalid_argument(message::invalid_filepath);
		}

		/* Input .cue path */
		// If the input is a file, check the extension
		if(system_vars.input_fstype == FilesystemType::File) {
			// Get the parent path of the passed file
			system_vars.input_dir_path = arg_filepath.parent_path() / "";

			// Make sure the file input is a .cue file
			if(StringToLower(arg_filepath.extension().string()) != ".cue") {
				throw std::invalid_argument(message::filepath_bad_extension);
			}

			system_vars.input_cue_path = arg_filepath;
		}

		// If the input is a path set the input_dir
		if(system_vars.input_fstype == FilesystemType::Directory) {
			system_vars.input_dir_path = arg_filepath / "";

			// Look for a .cue file in the directory. Error if one is not found
			system_vars.input_cue_path =
				FindFileWithExtension(system_vars.input_dir_path, ".cue");

			if(system_vars.input_cue_path.empty()) {
				throw std::invalid_argument(message::dir_missing_cue);
			}
		}


		/* Output dirctory path */
		// Set the output directory to the -d argument if given; if not, set it
		// to the input directory + /psx-comBINe/
		if(cli_handler.GetDetectedStatus(cli_args.dir_idx)) {
			system_vars.output_dir_path =
				std::filesystem::path(cli_handler.GetSubstring(cli_args.dir_idx)) / "";
		} else {
			system_vars.output_dir_path = system_vars.input_dir_path / "psx-comBINe" / "";
		}

		/* Output .cue and .bin */
		if(cli_handler.GetDetectedStatus(cli_args.file_idx)) {
			std::filesystem::path out_file_tmp(cli_handler.GetSubstring(cli_args.file_idx));

			// Make sure the filename is NOT a directory
			if(out_file_tmp.has_parent_path()) {
				throw std::invalid_argument(message::filename_has_dir);
			}

			// Make sure the extension is .cue
			if(StringToLower(out_file_tmp.extension().string()) != ".cue") {
				throw std::invalid_argument(message::filename_bad_extension);
			}

			// Append the custom .cue filename to the output .cue and .bin paths
			system_vars.output_cue_path = system_vars.output_dir_path / out_file_tmp;
			(system_vars.output_bin_path = system_vars.output_cue_path).replace_extension("bin");

		// If no override was passed, use the input .cue filename for output
		} else {
			system_vars.output_cue_path = system_vars.output_dir_path / system_vars.input_cue_path.filename();
			(system_vars.output_bin_path = system_vars.output_cue_path).replace_extension("bin");
		}

	} catch(const std::exception &e) {
		std::cerr << "Fatal Error: Filesystem: " << e.what() << "\n\n"
				  << message::short_help << std::endl;
		exit(EXIT_FAILURE);
	}
}


void CombineCue(SystemVariables &system_vars) {
	// Clear the cuesheet data
	sys_vars.input_cue_sheet.Clear();
	sys_vars.output_cue_sheet.Clear();

	// If the directory does not already exists, create it
	if(!std::filesystem::is_directory(system_vars.output_dir_path)) {
		std::filesystem::create_directory(system_vars.output_dir_path);
		std::cout << "Created Directory: " << system_vars.output_dir_path << "\n\n";
	}

	// Create an input and output .cue file handlers from the filesystem paths
	CueFile cue_in(system_vars.input_cue_path.string().c_str());
	CueFile cue_out(system_vars.output_cue_path.string().c_str());

	try {
		// Read the cue sheet data in, make sure there is at least one FILE
		cue_in.ReadCueData(system_vars.input_cue_sheet);
		if(system_vars.input_cue_sheet.FileList.empty()) {
			throw CueException(message::cue_has_no_files);
		}

		// Read the FILE sizes. Throws an exception on failure
		cue_in.GetCueFileSizes(system_vars.input_cue_sheet, system_vars.input_dir_path.string());

		// Copy the original sheet info to the combined sheet, then combine.
		system_vars.input_cue_sheet.CopyTo(system_vars.output_cue_sheet);
		system_vars.output_cue_sheet.Combine(system_vars.output_bin_path.filename().string(), "BINARY");

		// If the verbose flag was passed, print the combined sheet
		if(system_vars.verbose) system_vars.output_cue_sheet.Print();

		// Write the combined .cue file out
		cue_out.WriteCueData(system_vars.output_cue_sheet);

	} catch(const CueException &e) {
		std::cerr << "Fatal Error: Cue Handler: " << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}
}


std::string DumpBinaryFiles(SystemVariables &system_vars) {
	// Get the start millis
	std::chrono::milliseconds start_millis = GetMillisecs();

	// Create output binary file handler, and open the output file.
	// Create placeholder for input binary file handler
	std::fstream binary_file_in, binary_file_out;
	try {
		binary_file_out.open(
			system_vars.output_bin_path,
			std::ios::out | std::ios::binary | std::ios::trunc
		);

		if(!binary_file_out)
			throw std::runtime_error(message::output_bin_create_failed);

	} catch(const std::exception &e) {
		std::cerr << "Fatal Error: Creating " << system_vars.output_bin_path << ": "
				  << e.what() << std::endl;
		exit(EXIT_FAILURE);
	}

	// Print that dumping is beginning
	std::cout << "\n-------------------------------------------------------------------"
			  <<"\nDumping to " << system_vars.output_bin_path << "\n" << std::endl;

	// Keep track of bytes written in total and per file
	size_t total_output_bytes = 0, current_file_bytes = 0;

	// Create an array on the heap for the binary copy operations
	char *binary_array = new char[_BINARY_ARRAY_SIZE];

	/* loop */
	// Go through every file in the input cue sheet, dumping data to the output binary file
	for(const auto &f_itr : system_vars.input_cue_sheet.FileList) {

		// Get the filepath for the current binary file, then try to open the file
		std::filesystem::path current_binary_path(system_vars.input_dir_path / f_itr.filename);
		try {
			binary_file_in.open(
				current_binary_path,
				std::ios::in | std::ios::binary
			);

			if(!binary_file_in) std::runtime_error(message::input_bin_not_open);
		} catch(const std::exception &e) {
			std::cerr << "Fatal Error: Opening input binary file "
					  << current_binary_path << ": " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}

		// Print which file is being worked on
		std::cout << "Dumping File " << current_binary_path << std::flush;

		// Reset the file flags and go to the beginning
		binary_file_in.clear();
		binary_file_in.seekg(0, std::ios::beg);

		// Reset the number of bytes read for this file
		current_file_bytes = 0;

		// Copy chunks from the input to the output file, until all bytes are copied
		std::streamsize buffer_bytes = 0;
		do {
			// Read chunk from input
			binary_file_in.read(binary_array, _BINARY_ARRAY_SIZE);
			buffer_bytes = binary_file_in.gcount();

			// Write chunk to output
			binary_file_out.write(binary_array, buffer_bytes);

			// Add the bytes read this loop to the current file bytes
			current_file_bytes += static_cast<size_t>(buffer_bytes);
		} while(buffer_bytes);


		// Add this files bytes to the total
		total_output_bytes += current_file_bytes;

		// Report how many MiBs were copied for this file
		std::cout << BytesToPaddedMiBString(current_file_bytes, 6) << std::endl;

		// Close this file for the next loop
		binary_file_in.close();
	}

	/* loop done */
	// Clear the RAM Buffer and close the output file
	delete[] binary_array;
	binary_file_out.close();

	// Get the end Milliseconds, and calculate how long it took to finish
	std::chrono::milliseconds end_millis = GetMillisecs();
	float runtime =
		static_cast<float>((end_millis - start_millis).count()) / 1000.0f;

	// Create output message
	std::stringstream stream;
	stream << "Successfully Dumped "
		   << BytesToPaddedMiBString(total_output_bytes, 0)
		   << " in " << std::fixed << std::setprecision(2) << runtime
		   << " seconds." << std::endl;

	return stream.str();
}
