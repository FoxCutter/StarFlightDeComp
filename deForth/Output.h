#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct FieldInfo
{
	int Overlay = 0;
	std::string Name;
	uint8_t File = 0;
	uint8_t Offset = 0;
	uint8_t Length = 0;

	char Type = 0;
};

extern std::vector<FieldInfo> KnowFields;
struct FileInformation;

void DumpKnownFields(const std::filesystem::path& OutputPath, std::string FileName);
void DumpRenames(const std::filesystem::path& OutputPath, std::string FileName);

void DumpGeneric(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpStringData(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File, char Trim = ' ', bool Length = false);

void DumpMain(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpOverlay(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);

void DumpDirectory(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpVessel(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpArtifact(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpAnalyzeText(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpButtons(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpRegions(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpPlanetData(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpColorMap(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpEarth(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpElements(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpCompounds(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpCrewmember(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpPStats(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);

void DumpFullScreenImage(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpCrewPic(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpLayerPic(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpIcons(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpSystemIcon(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpVesBlt(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpFonts(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpPhazes(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);
void DumpGalaxy(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);

void DumpWordlist(const std::filesystem::path& OutputPath, std::string FileName, FileInformation& File);