#pragma once

#pragma pack(push, 1)
struct OverlayHeader
{
	uint16_t Start;			// Overlay ID, PtrAddress of file in Paragraphs
	uint16_t Size;			// Overlay Length in Paragraphs
	uint16_t LoadAddress;	// Overlay Base PtrAddress
	uint16_t DataPtr;		// The DP of the overlay
	uint16_t Vocabulary;	// PtrAddress of the VOCABULARY word for this overlay
	uint16_t Link[4];		// Dictionary Links
	uint16_t DataEnd;		// End of the overlay data in Paragraphs
};


struct DirectoryInfoEntry
{
	char Name[12];
	uint8_t Disk;
	uint16_t Start;		// In Paragraphs
	uint16_t End;		// In Paragraphs
	uint16_t RecodCount;
	uint8_t RecordLen;
	uint8_t InstanceLen;
};

struct IAdder
{
	uint16_t Low;
	uint8_t High;

	uint32_t Full() const
	{
		return (High << 16) | Low;
	}
};

struct ButtonData
{
	char Text[6][12];
	uint8_t Count;
};


struct ArtifactData
{
	char Name[24];
	uint8_t AnalyzeTextID;
	uint16_t Volume;
	uint16_t Value;
	uint8_t Analyzed;
};

struct AnalyzeTextData
{
	char Text[5][38];
};

struct CMAPData
{
	uint8_t Color[8][8];
};

struct ElementData
{
	char Name[16];
	uint16_t Value;	

	// These fields don't seem to be used, so just making a reandom guess
	uint8_t Weight;
	uint8_t filler;
	uint16_t Volume;
};

struct CompoundData
{
	IAdder Name;
	uint8_t filler[4];
};


struct RegionData
{
	uint16_t Filler;
	int16_t LSeedAdjustment;
	IAdder Name;
	uint8_t v07[8];
	uint8_t v15;
	uint8_t v16;
	uint8_t v17;
	uint8_t v18;
	uint8_t v19[8];
	uint8_t v27;

};

struct PlanetData
{
	uint8_t MassType;			// 0
	uint8_t SurfaceType;		// 1
	uint16_t Mass;				// 2
	uint8_t LSeed;				// 4
	uint16_t RegionSeed;		// 5
	uint16_t GlobalSeed;		// 7
	uint8_t MineralDensity;		// 9
	uint8_t Seed4;				// 10
	uint8_t Seed5;				// 11
	uint8_t Minerals[3];		// 12
	uint8_t Coldest;			// 15
	uint8_t Warmest;			// 16
	uint16_t AtmoActivity;		// 17
	uint8_t AtmoDensity;		// 19
	uint8_t Narrowest;			// 20
	uint8_t Flattest;			// 21
};


struct CrewMemberData
{
	uint8_t SpeciesLen;			// v0
	char Species[7];			// v1
	uint8_t Stats[5];			// v8
	uint8_t MaxStats[5];		// v13
	uint8_t LearningRate;		// v19
	uint8_t Durability;			// v18
};

struct PStatsData
{
	char Type[9];
	uint8_t AvgHeight;
	uint8_t AvgWeight;
};


struct VesselData
{
	uint8_t Species;
	IAdder Image;
	uint8_t Class;
	uint8_t Power;
	uint8_t Speed;
	uint16_t Mass;
	uint16_t TempMax;
	uint8_t RateOfFire;
	uint16_t Armor;
	uint16_t Shield;
	uint8_t MissleHitFlag;
	uint8_t LaserHitFlag;
	uint8_t LaserClass;
	uint8_t MissileClass;
	uint8_t PlasmaClass;
	uint8_t Element[3];
	uint8_t EnduriumDebris;
	uint8_t Debris[3];
	uint8_t Directional;
	uint8_t SensorData[7][6];
};

struct Instance
{
	IAdder Sibling;
	IAdder Prev;
	IAdder Offset;
	uint8_t Class;
	uint8_t SubClass;
};

struct CrewmemberInstance
{
	char Name[15];
	uint8_t Stats[5];
	uint8_t Heath;
	uint16_t Status;			// Flags 0x8 = Assigned? 0x4 = Treating, 0x2 = Dead, 0x1 = in use?
	uint8_t Index;
};

struct AssignCrewInstance
{
	uint16_t Unused[3];
	IAdder Captian;
	IAdder Science;
	IAdder Navigator;
	IAdder Engineer;
	IAdder Communication;
	IAdder Doctor;
};

struct BankInstance
{
	uint16_t PreTransactionBallance[2];	// Stored as a Double Word
	uint16_t Ballance[2];				// Stored as a Double Word
	uint8_t TransactionFlag;
};

struct BankTransactionInstance
{
	uint16_t Amount[2];					// Stored as a Double Word
	uint16_t Stardate;
};

struct ShipInstance
{
	uint16_t Unused;		// 0
	uint16_t PosX;			// 2
	uint16_t PosY;			// 4

	uint16_t ArmorClass;	// 17
	uint16_t EngineClass;	// 19
	uint16_t Sensors;		// 21
	uint16_t Comm;			// 23
	uint16_t ShieldClass;	// 25
	uint16_t MissileClass;	// 27
	uint16_t LaserClass;	// 29
	uint16_t Pods;			// 31
	uint16_t Unused2[2];	// 34
	uint16_t Acceleration;	// 37
	uint16_t Mass;			// 39
	uint8_t Repairs[7];		// 40
	uint16_t Shields;		// 48
	uint16_t Armor;			// 50
	char Name[15];			// 52
	uint16_t Cargo;			// 67
	uint16_t Flags[2];		// 69	Not sure why it's 4 byte, seems to only use the first one
	uint8_t Icon;			// 73
};

struct StarsytemInstance
{
	uint16_t FlareDate;
	uint16_t PosX;
	uint16_t PosY;
	uint8_t Orbits;
	uint8_t Reports;
};

struct EcounterInstance
{
	uint8_t Homeword;
	uint8_t Unused;
	uint16_t PosX;
	uint16_t PosY;
	int8_t DistX;
	int8_t DistY;
	uint8_t FleetSize;
	uint8_t ShipsDestroyed;
	uint8_t MaxShipsAtATime;
	uint8_t Vessels[4];
	uint8_t Unused2;
	uint8_t Enabled;
};

struct OriginatorInstance
{
	uint8_t Color;
	uint8_t TalkChance;
	uint8_t Flag;		// Can surrender?
	uint8_t Filler[4];	// These have values, but don't seem to be used
	uint8_t InitalPosture;
	uint8_t FriendlyThreshold;
	uint8_t NeutralThreshold;
	uint8_t HostileThreshold;
	int8_t ObsequiousEffect;
	int8_t FriendEffect;
	int8_t HostileEffect;
	uint8_t MaxWords;
	IAdder Words;
	uint8_t PictureNumber;
};

struct PhaseControlInstance
{
	uint8_t Context;
	uint8_t OriginatorPostureType;
	uint8_t SentCount;
	int8_t Posture_Always;
	int8_t Posture_No;
	int8_t Posture_Yes;
};



#pragma pack(pop)
