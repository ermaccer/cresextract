// cresextract.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include "filef.h"
#include <filesystem>
#pragma comment (lib, "zlib/zdll.lib" )
#include "zlib\zlib.h"

struct CRESHeader {
	// wide string Centauri Production Resource File 3.10
	int        files;
	int        fileStructOffset;
};

struct CRESEntry {
	//wide string name
	char      pad[12];
	int       offset;
	int       size[2];
	char      _pad[4];
};

enum eModes {
	MODE_EXTRACT = 1
};

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cout << "Usage: crestool <params> <file>\n"
			<< "    -o  Specifies a folder for extraction\n"
			<< "Example: \n"
			<< "cresextract -e patamat_music.res - extracts archive" << std::endl;;
		return 1;
	}

	std::string folder;
	int mode = 0;
	// params
	for (int i = 1; i < argc - 1; i++)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) {
			return 1;
		}
		switch (argv[i][1])
		{
		case 'e': mode = MODE_EXTRACT;
			break;
		case 'o':
			i++;
			folder = argv[i];
			break;
		default:
			std::cout << "ERROR: Param does not exist: " << argv[i] << std::endl;
			break;
		}
	}
	if (mode == MODE_EXTRACT)
	{
		std::ifstream pFile(argv[argc - 1], std::ifstream::binary);

		if (!pFile)
		{
			std::cout << "ERROR: Could not open: " << argv[argc - 1] << "!" << std::endl;
			return 1;
		}
		if (pFile)
		{
			std::string header = getWideStr(pFile,true);
			if (header !="Centauri Production Resource File 3.10")
			{
				std::cout << "ERROR: " << argv[argc - 1] << " is not a valid Centauri Archive!" << std::endl;
				return 1;
			}
			CRESHeader cres;
			pFile.read((char*)&cres, sizeof(CRESHeader));


			std::unique_ptr<int[]> offset = std::make_unique<int[]>(cres.files);
			std::unique_ptr<int[]> size = std::make_unique<int[]>(cres.files);
			std::unique_ptr<int[]> sizeCmp = std::make_unique<int[]>(cres.files);
			std::unique_ptr<std::string[]> name = std::make_unique<std::string[]>(cres.files);

			pFile.seekg(cres.fileStructOffset,pFile.beg);

			for (int i = 0; i < cres.files; i++)
			{
				name[i] = getWideStr(pFile);
				CRESEntry ent;
				pFile.read((char*)&ent, sizeof(CRESEntry));
				offset[i] = ent.offset;
				size[i] = ent.size[0];
				sizeCmp[i] = ent.size[1];

			}

			// extract

			for (int i = 0; i < cres.files; i++)
			{
				// create output
				std::string output = name[i];

				if (!folder.empty())
				{
					output.insert(0, folder);
					output.insert(folder.length(), "\\");
				}
				std::ofstream oFile(output, std::ofstream::binary);

				std::cout << "Processing: " << name[i] << std::endl;
				pFile.seekg(offset[i], pFile.beg);

				std::unique_ptr<char[]> dataBuff = std::make_unique<char[]>(size[i]);
				pFile.read(dataBuff.get(), size[i]);

				// decompress data
				std::unique_ptr<char[]> uncompressedBuffer = std::make_unique<char[]>(size[i]);
				unsigned long uncompressedSize = size[i];
				int zlib_output = uncompress((Bytef*)uncompressedBuffer.get(), &uncompressedSize,
					(Bytef*)dataBuff.get(), sizeCmp[i]);

				if (zlib_output == Z_MEM_ERROR) {
					std::cout << "ERROR: ZLIB: Out of memory!" << std::endl;
					return 1;
				}
				if (checkSlash(name[i]))
			     	std::experimental::filesystem::create_directories(splitString(output, false));
				oFile.write(uncompressedBuffer.get(), size[i]);
			}
		}
	}

	return 0;
}

