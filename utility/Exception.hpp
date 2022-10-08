#pragma once

#include "Common.hpp"

#include <exception>
#include <sstream>
#include <string>

#ifdef WIN32
#    include <Windows.h>
#    define THROW_BASE(r)                                                 \
        throw utility::exception(::GetLastError(), __FILE__, __FUNCSIG__, \
                                 __LINE__, r)
#else
#    define THROW_BASE(r) throw utility::exception(__FILE__, __func__, __LINE__, r)
#endif

#define THROW_MSG(x, r) THROW_BASE(r).Message(x)
#define THROW(r) THROW_BASE(r)

namespace {
    std::string result_to_error_message(Result result)
    {
        switch (result) {
            case Result::SUCCESS:
                return "Success";
            case Result::UNRECOGNIZED_EXTENSION:
                return "Unrecognized extension";
            case Result::NO_MOGP_CHUNK:
                return "No MOGP chunk";
            case Result::NO_MOPY_CHUNK:
                return "No MOPY chunk";
            case Result::NO_MOVI_CHUNK:
                return "No MOVI chunk";
            case Result::NO_MOVT_CHUNK:
                return "No MOVT chunk";
            case Result::UNRECOGNIZED_DBC_FILE:
                return "Unrecognized DBC file";
            case Result::MVER_NOT_FOUND:
                return "MVER not found";
            case Result::MOMO_NOT_FOUND:
                return "MOMO not found";
            case Result::MOHD_NOT_FOUND:
                return "MOHD not found";
            case Result::MODS_NOT_FOUND:
                return "MODS not found";
            case Result::MODN_NOT_FOUND:
                return "MODN not found";
            case Result::MODD_NOT_FOUND:
                return "MODD not found";
            case Result::EMPTY_WMO_DOODAD_INSTANTIATED:
                return "Empty WMO doodad instantiated";
            case Result::FAILED_TO_OPEN_FILE_FOR_BINARY_STREAM:
                return "Failed to open file for BinaryStream";
            case Result::WDT_OPEN_FAILED:
                return "WDT open failed";
            case Result::MPHD_NOT_FOUND:
                return "MPHD not found";
            case Result::MAIN_NOT_FOUND:
                return "MAIN not found";
            case Result::MDNM_NOT_FOUND:
                return "MDNM not found";
            case Result::MONM_NOT_FOUND:
                return "MONM not found";
            case Result::MWMO_NOT_FOUND:
                return "MWMO not found";
            case Result::MODF_NOT_FOUND:
                return "MODF not found";
            case Result::BAD_FORMAT_OF_GAMEOBJECT_FILE:
                return "Bad format of gameobject file";
            case Result::NO_MCNK_CHUNK:
                return "No MCNK chunk";
            case Result::MHDR_NOT_FOUND:
                return "MHDR not found";
            case Result::FAILED_TO_OPEN_ADT:
                return "Failed to open ADT";
            case Result::ADT_VERSION_IS_INCORRECT:
                return "ADT version is incorrect";
            case Result::MVER_DOES_NOT_BEGIN_ADT_FILE:
                return "MVER does not begin ADT file";
            case Result::BAD_MATRIX_ROW:
                return "Bad row";
            case Result::INVALID_MATRIX_MULTIPLICATION:
                return "Invalid matrix multiplication";
            case Result::ONLY_4X4_MATRIX_IS_SUPPORTED:
                return "Only 4x4 matrix is supported";
            case Result::MATRIX_NOT_INVERTIBLE:
                return "Matrix is not invertible!";
            case Result::UNEXPECTED_VERSION_MAGIC_IN_ALPHA_MODEL:
                return "Unexpected version magic in alpha model";
            case Result::UNEXPECTED_VERSION_SIZE_IN_ALPHA_MODEL:
                return "Unexpected version size in alpha model";
            case Result::UNSUPPORTED_ALPHA_MODEL_VERSION:
                return "Unsupported alpha model version";
            case Result::UNEXPECTED_VERTEX_MAGIC_IN_ALPHA_MODEL:
                return "Unexpected vertex magic in alpha model";
            case Result::UNEXPECTED_TRIANGLE_MAGIC_IN_ALPHA_MODEL:
                return "Unexpected triangle magic in alpha model";
            case Result::INVALID_DOODAD_FILE:
                return "Invalid doodad file";
            case Result::FAILED_TO_OPEN_GAMEOBJECT_FILE:
                return "Failed to open gameobject file";
            case Result::UNRECOGNIZED_MODEL_EXTENSION:
                return "Unrecognized model extension";
            case Result::GAME_OBJECT_REFERENCES_NON_EXISTENT_MODEL_ID:
                return "Gameobject references non-existent model id";
            case Result::ADT_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE:
                return "ADT serialization failed to open output file";
            case Result::WMO_SERIALIZATION_FAILED_TO_OPEN_OUTPUT_FILE:
                return "WMO serialization failed to open output file";
            case Result::REQUESTED_BVH_NOT_FOUND:
                return "Requested BVH not found";
            case Result::COULD_NOT_OPEN_MPQ:
                return "Could not open MPQ";
            case Result::GETROOTAREAID_INVALID_ID:
                return "GetRootAreaId invalid id";
            case Result::NO_DATA_FILES_FOUND:
                return "No data files found";
            case Result::MPQ_MANAGER_NOT_INIATIALIZED:
                return "MpqManager not initialized";
            case Result::ERROR_IN_SFILEOPENFILEX:
                return "Error in SFileOpenFileEx";
            case Result::ERROR_IN_SFILEREADFILE:
                return "Error in SFileReadFile";
            case Result::MULTIPLE_CANDIDATES_IN_ALPHA_MPQ:
                return "Multiple candidates in alpha MPQ";
            case Result::TOO_MANY_FILES_IN_ALPHA_MPQ:
                return "Too many files in alpha MPQ";
            case Result::NO_MPQ_CANDIDATE:
                return "No candidate. Target file size 16?";
            case Result::BVH_INDEX_FILE_NOT_FOUND:
                return "BVH index file not found";
            case Result::INCORRECT_FILE_SIGNATURE:
                return "Incorrect file signature";
            case Result::INCORRECT_FILE_VERSION:
                return "Incorrect file version";
            case Result::NOT_WMO_NAV_FILE:
                return "Not WMO nav file";
            case Result::NOT_WMO_TILE_COORDINATES:
                return "Not WMO tile coordinates";
            case Result::NOT_ADT_NAV_FILE:
                return "Not ADT nav file";
            case Result::INVALID_MAP_FILE:
                return "Invalid map file";
            case Result::INCORRECT_WMO_COORDINATES:
                return "Incorrect WMO coordinates";
            case Result::DTNAVMESHQUERY_INIT_FAILED:
                return "dtNavMeshQuery::init failed";
            case Result::UNKNOWN_WMO_INSTANCE_REQUESTED:
                return "Unknown WMO instance requested";
            case Result::UNKNOWN_DOODAD_INSTANCE_REQUESTED:
                return "Unknown doodad instance requested";
            case Result::COULD_NOT_DESERIALIZE_DOODAD:
                return "Could not deserialize doodad";
            case Result::COULD_NOT_DESERIALIZE_WMO:
                return "Could not deserialize WMO";
            case Result::INCORRECT_ADT_COORDINATES:
                return "Incorrect ADT coordinates";
            case Result::TILE_NOT_FOUND_FOR_REQUESTED:
                return "Tile not found for requested (x, y)";
            case Result::BINARYSTREAM_COMPRESS_FAILED:
                return "BinaryStream::Compress failed";
            case Result::MZ_INFLATEINIT_FAILED:
                return "mz_inflateInit failed";
            case Result::MZ_INFLATE_FAILED:
                return "mz_inflate failed";
            case Result::INVALID_ROW_COLUMN_FROM_DBC:
                return "Invalid row, column requested from DBC";
            case Result::GAMEOBJECT_WITH_SPECIFIED_GUID_ALREADY_EXISTS:
                return "Game object with specified GUID already exists";
            case Result::TEMPORARY_WMO_OBSTACLES_ARE_NOT_SUPPORTED:
                return "Temporary WMO obstacles are not supported";
            case Result::NO_DOODAD_SET_SPECIFIED_FOR_WMO_GAME_OBJECT:
                return "No doodad set specified for WMO game object";

            default:
                return "Unknown error";
        }
    }
}

namespace utility
{
class exception : public virtual std::exception
{
private:
    std::string _str;
    const Result _result;


#ifdef WIN32
    unsigned int _err;
#endif

public:
#ifdef WIN32
    exception(unsigned int err, const char* file, const char* function,
              int line, Result result)
        : _err(err)
        , _result(result)
#else
    exception(const char* file, const char* function, int line, Result result)
        : _result(result)
#endif
    {
        std::stringstream s;

        s << file << ":" << line << " (" << function << "): " << result_to_error_message(result);

        _str = s.str();
    }

#ifdef WIN32
    exception& ErrorCode()
    {
        char buff[1024];
        constexpr size_t buffSize = sizeof(buff) / sizeof(buff[0]);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       nullptr, _err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buff, buffSize, nullptr);

        std::stringstream s;

        s << _str << " error " << _err << " (" << buff << ")";

        _str = s.str();

        return *this;
    }
#else
    exception& ErrorCode() { return *this; }
#endif

    exception& Message(const std::string& msg)
    {
        std::stringstream s;

        s << _str << " " << msg;

        _str = s.str();

        return *this;
    }

    const char* what() const noexcept override { return _str.c_str(); }

    Result ResultCode() const noexcept { return _result; }
};
} // namespace utility