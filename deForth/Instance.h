#pragma once

#include <stdint.h>
#include "String.h"
#include <vector>
#include "files.h"
#include <filesystem>

void InitilizeInstanceData(FileInformation& InstanceFile);
void ProcessInstanceData();

bool ValidInstanceRecord(uint32_t IAddress);

std::string GetInstanceRecordValue(uint32_t IAddress);

void DumpInstanceData(const std::filesystem::path& OutputPath);
void DumpUniverseData(const std::filesystem::path& OutputPath);
void DumpBoxDataTree(const std::filesystem::path& OutputPath, std::string BoxName);
void DumpClassDataTree(const std::filesystem::path& OutputPath, std::string ClassName);
