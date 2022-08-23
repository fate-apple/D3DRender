#include "pch.h"
#include "Image.h"

#define NANOSVG_IMPLEMENTATION
#include <nanosvg/nanosvg.h>

#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

#include "Log.h"

static bool TryLoadFromCache(const fs::path& filepath, uint32 flags, fs::path& cacheFilePath,
                             DirectX::ScratchImage& scratchImage, DirectX::TexMetadata& metadata)
{
    fs::path cachedFilename = cacheFilePath;
    cachedFilename.replace_extension("." + std::to_string(flags) + ".cache.dds");
    cacheFilePath = L"asset_cache" / cachedFilename;
    bool fromCache = false;
    if(! (flags & image_load_flags_always_load_from_source)) {
        WIN32_FILE_ATTRIBUTE_DATA cachedData;
        //Ex => more infomation, W => write access : get attributeData reference
        if(GetFileAttributesExW(cacheFilePath.c_str(), GetFileExInfoStandard, &cachedData)) {
            FILETIME cachedFileTime = cachedData.ftLastWriteTime;
            WIN32_FILE_ATTRIBUTE_DATA oriData;
            assert(GetFileAttributesExW(filepath.c_str(), GetFileExInfoStandard, &oriData));
            FILETIME oriFileTime = oriData.ftLastWriteTime;
            if(CompareFileTime(&cachedFileTime, &oriFileTime) >= 0) {
                // Cached file is newer than original
                fromCache = SUCCEEDED(
                    DirectX::LoadFromDDSFile(cacheFilePath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage));
            }
        }
    }
    return fromCache;
}


static DXGI_FORMAT MakeSRGB(DXGI_FORMAT format)
{
    return DirectX::MakeSRGB(format);
}

static DXGI_FORMAT MakeLinear(DXGI_FORMAT format)
{
    switch(format) {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case DXGI_FORMAT_BC1_UNORM_SRGB:
        format = DXGI_FORMAT_BC1_UNORM;
        break;

    case DXGI_FORMAT_BC2_UNORM_SRGB:
        format = DXGI_FORMAT_BC2_UNORM;
        break;

    case DXGI_FORMAT_BC3_UNORM_SRGB:
        format = DXGI_FORMAT_BC3_UNORM;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        format = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        format = DXGI_FORMAT_B8G8R8X8_UNORM;
        break;

    case DXGI_FORMAT_BC7_UNORM_SRGB:
        format = DXGI_FORMAT_BC7_UNORM;
        break;
    }
    return format;
}

/**
 * \brief standard Rgb format, gen mipmaps, premultiply alpha, decompress single pixel to 4x4 block(icon), compress use BC algorithm, cache to dds.
 */
static void PostProcessImage(DirectX::ScratchImage& scratchImage, DirectX::TexMetadata& metadata, uint32 flags,
                             const fs::path& filepath, const fs::path& cacheFilepath)
{
    if(flags & image_load_flags_noncolor) {
        metadata.format = MakeLinear(metadata.format);
    } else {
        metadata.format = MakeSRGB(metadata.format);
    }
    scratchImage.OverrideFormat(metadata.format);
    if(flags & image_load_flags_gen_mips_on_cpu && !DirectX::IsCompressed(metadata.format)) {
        DirectX::ScratchImage mipchainImage;

        CheckResult(DirectX::GenerateMipMaps(scratchImage.GetImages(), scratchImage.GetImageCount(), metadata,
                                             DirectX::TEX_FILTER_DEFAULT, 0, mipchainImage));
        scratchImage = std::move(mipchainImage);
        metadata = scratchImage.GetMetadata();
    } else {
        metadata.mipLevels = max(1u, (uint32)metadata.mipLevels);
    }
    //This converts an image to using premultiplied alpha. Note the format and size are not changed.
    if(flags & image_load_flags_premultiply_alpha) {
        DirectX::ScratchImage premultipliedAlphaImage;

        CheckResult(DirectX::PremultiplyAlpha(scratchImage.GetImages(), scratchImage.GetImageCount(), metadata,
                                              DirectX::TEX_PMALPHA_DEFAULT, premultipliedAlphaImage));
        scratchImage = std::move(premultipliedAlphaImage);
        metadata = scratchImage.GetMetadata();
    }
    //Compressed single pixel back to 4x4 block
    if(DirectX::IsCompressed(metadata.format) && metadata.width == 1 && metadata.height == 1) {
        assert(metadata.mipLevels == 1);
        assert(scratchImage.GetImageCount() == 1);

        DirectX::TexMetadata scaledMetadata = metadata;
        scaledMetadata.width = 4;
        scaledMetadata.height = 4;
        scaledMetadata.mipLevels = 1;

        DirectX::ScratchImage scaledImage;
        CheckResult(scaledImage.Initialize(scaledMetadata));

        memcpy(scaledImage.GetImage(0, 0, 0)->pixels, scratchImage.GetImage(0, 0, 0)->pixels, scratchImage.GetPixelsSize());
        scratchImage = std::move(scaledImage);
        metadata = scratchImage.GetMetadata();
    }
    if(flags & image_load_flags_compress && !DirectX::IsCompressed(metadata.format)) {
        if(metadata.width % 4 == 0 && metadata.height % 4 == 0) {
            uint32 numChannels = getNumberOfChannels(metadata.format);

            DXGI_FORMAT compressedFormat;

            switch(numChannels) {
            case 1: compressedFormat = DXGI_FORMAT_BC4_UNORM;
                break;
            case 2: compressedFormat = DXGI_FORMAT_BC5_UNORM;
                break;

            case 3:
            case 4:
                {
                    if(scratchImage.IsAlphaAllOpaque()) {
                        compressedFormat = DirectX::IsSRGB(metadata.format)
                                               ? DXGI_FORMAT_BC1_UNORM_SRGB
                                               : DXGI_FORMAT_BC1_UNORM;
                    } else {
                        compressedFormat = DirectX::IsSRGB(metadata.format)
                                               ? DXGI_FORMAT_BC3_UNORM_SRGB
                                               : DXGI_FORMAT_BC3_UNORM;
                    }
                }
                break;
            }

            DirectX::ScratchImage compressedImage;

            CheckResult(DirectX::Compress(scratchImage.GetImages(), scratchImage.GetImageCount(), metadata,
                                          compressedFormat, DirectX::TEX_COMPRESS_PARALLEL, DirectX::TEX_THRESHOLD_DEFAULT,
                                          compressedImage));
            scratchImage = std::move(compressedImage);
            metadata = scratchImage.GetMetadata();
        } else {
            LOG_ERROR("Cannot compress texture '%ws', since its dimensions are not a multiple of 4", filepath.c_str());
            std::cerr << "PostProcessImage : Cannot compress texture '" << filepath <<
            "', since its dimensions are not a multiple of 4.\n";
        }
    }
    if(flags & image_load_flags_cache_to_dds) {
        fs::create_directories(cacheFilepath.parent_path());
        CheckResult(DirectX::SaveToDDSFile(scratchImage.GetImages(), scratchImage.GetImageCount(), metadata,
                                           DirectX::DDS_FLAGS_NONE, cacheFilepath.c_str()));
    }
}

static void CreateDesc(DirectX::TexMetadata& metadata, uint32 flags, D3D12_RESOURCE_DESC& texDesc)
{
    if(flags & image_load_flags_allocate_full_mipchain) {
        metadata.mipLevels = 0;
    }
    switch(metadata.dimension) {
    case DirectX::TEX_DIMENSION_TEXTURE1D:
        texDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, metadata.width, (uint16)metadata.arraySize,
                                               (uint16)metadata.mipLevels);
        break;
    case DirectX::TEX_DIMENSION_TEXTURE2D:
        texDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, (uint32)metadata.height,
                                               (uint16)metadata.arraySize, (uint16)metadata.mipLevels);
        break;
    case DirectX::TEX_DIMENSION_TEXTURE3D:
        texDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, metadata.width, (uint32)metadata.height, (uint16)metadata.depth,
                                               (uint16)metadata.mipLevels);
        break;
    default:
        assert(false);
        break;
    }
}

/**
 * \brief use nanosvg to read and Rasterize svg image.
 * For DirectXTex functions that create new images, the results are returned in an instance of ScratchImage which is a 'container' for Image instances:
 */
bool loadSVGFromFile(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratchImage,
                     D3D12_RESOURCE_DESC& textureDesc)
{
    fs::path cacheFilePath;
    DirectX::TexMetadata metadata;
    bool fromCache = TryLoadFromCache(filepath, flags, cacheFilePath, scratchImage, metadata);

    if(!fromCache) {
        if(!fs::exists(filepath)) {
            std::cerr << "loadSVGFromFile: Could not find file" << filepath.c_str() << std::endl;
            return false;
        }
        NSVGimage* svg = nsvgParseFromFile(filepath.string().c_str(), "px", 96);
        uint32 width = static_cast<uint32>(ceil(svg->width));
        uint32 height = static_cast<uint32>(ceil(svg->height));

        uint8* rawImage = new uint8[width * height * 4];
        NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
        nsvgRasterize(rasterizer, svg, 0, 0, 1, rawImage, width, height, width * 4);
        nsvgDeleteRasterizer(rasterizer);
        nsvgDelete(svg);

        DirectX::Image dxImage = {width, height, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, width * 4, width * height * 4, rawImage};
        scratchImage.InitializeFromImage(dxImage);
        metadata = scratchImage.GetMetadata();
        PostProcessImage(scratchImage, metadata, flags, filepath, cacheFilePath);
        delete[] rawImage;
    }
    CreateDesc(metadata, flags, textureDesc);

    return true;
}


bool loadImageFromFile(const fs::path& filepath, uint32 flags, DirectX::ScratchImage& scratchImage,
                       D3D12_RESOURCE_DESC& textureDesc)
{
    fs::path cacheFilepath;
    DirectX::TexMetadata metadata;
    bool fromCache = TryLoadFromCache(filepath, flags, cacheFilepath, scratchImage, metadata);

    if(!fromCache) {
        if(!fs::exists(filepath)) {
            std::cerr << "loadImageFromFile: Could not find file '" << filepath.string() << "'.\n";
            return false;
        }

        fs::path extension = filepath.extension();
        if(extension == ".dds") {
            if(FAILED(DirectX::LoadFromDDSFile(filepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage))) {
                return false;
            }
        } else if(extension == ".hdr") {
            if(FAILED(DirectX::LoadFromHDRFile(filepath.c_str(), &metadata, scratchImage))) {
                return false;
            }
        } else if(extension == ".tga") {
            if(FAILED(DirectX::LoadFromTGAFile(filepath.c_str(), &metadata, scratchImage))) {
                return false;
            }
        } else {
            //.BMP, .PNG, .GIF, .TIFF, .JPEG
            if(FAILED(DirectX::LoadFromWICFile(filepath.c_str(), DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratchImage))) {
                return false;
            }
        }

        PostProcessImage(scratchImage, metadata, flags, filepath, cacheFilepath);
    }

    CreateDesc(metadata, flags, textureDesc);

    return true;
}

//used by assimp load
bool loadImageFromMemory(const void* data, uint32 size, image_format imageFormat, const fs::path& cachingFilepath, uint32 flags,
                         DirectX::ScratchImage& scratchImage, D3D12_RESOURCE_DESC& textureDesc)
{
    if (flags & image_load_flags_gen_mips_on_gpu)
    {
        flags &= ~image_load_flags_gen_mips_on_cpu;
        flags |= image_load_flags_allocate_full_mipchain;
    }
    fs::path cachedFilename = cachingFilepath.string() + std::to_string(flags) + ".cache.dds";
    fs::path cacheFilepath = L"asset_cache" / cachedFilename;
    bool fromCache = false;
    DirectX::TexMetadata metadata;
    if (!(flags & image_load_flags_always_load_from_source))
    {
        // Look for cached.

        if (fs::exists(cacheFilepath))
        {
            fromCache = SUCCEEDED(DirectX::LoadFromDDSFile(cacheFilepath.c_str(), DirectX::DDS_FLAGS_NONE, &metadata, scratchImage));
        }
    }
    if (!fromCache)
    {
        if (flags & image_load_flags_cache_to_dds)
        {
            LOG_MESSAGE("Preprocessing in-memory texture '%ws' for faster loading next time", cachingFilepath.c_str());
            std::cout << "Preprocessing in-memory texture '" << cachingFilepath.string() << "' for faster loading next time.";
#ifdef _DEBUG
            std::cout << " Consider running in a release build the first time.";
#endif
            std::cout << '\n';
        }


        if (imageFormat == image_format_dds)
        {
            if (FAILED(DirectX::LoadFromDDSMemory(data, size, DirectX::DDS_FLAGS_NONE, &metadata, scratchImage)))
            {
                return false;
            }
        }
        else if (imageFormat == image_format_hdr)
        {
            if (FAILED(DirectX::LoadFromHDRMemory(data, size, &metadata, scratchImage)))
            {
                return false;
            }
        }
        else if (imageFormat == image_format_tga)
        {
            if (FAILED(DirectX::LoadFromTGAMemory(data, size, &metadata, scratchImage)))
            {
                return false;
            }
        }
        else
        {
            assert(imageFormat == image_format_wic);
            //.BMP, .PNG, .GIF, .TIFF, .JPEG
            if (FAILED(DirectX::LoadFromWICMemory(data, size, DirectX::WIC_FLAGS_FORCE_RGB, &metadata, scratchImage)))
            {
                return false;
            }
        }

        PostProcessImage(scratchImage, metadata, flags, cacheFilepath, cacheFilepath);
    }

    CreateDesc(metadata, flags, textureDesc);

    return true;
}