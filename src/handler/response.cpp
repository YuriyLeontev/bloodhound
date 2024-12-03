#include "response.h"

namespace http_request {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;

const std::unordered_map<std::string_view, std::string_view> ContentType::ext_to_content =
{
    {".htm"sv,  TEXT_HTML},
    {".html"sv, TEXT_HTML},
    {".css"sv,  TEXT_CSS},
    {".txt"sv,  TEXT_PLAIN},
    {".js"sv,   TEXT_JS},
    {".json"sv, APPLICATION_JSON},
    {".xml"sv,  APPLICATION_XML},
    {".png"sv,  IMAGE_PNG},
    {".jpg"sv,  IMAGE_JPG},
    {".jpe"sv,  IMAGE_JPG},
    {".jpeg"sv, IMAGE_JPG},
    {".gif"sv,  IMAGE_GIF},
    {".bmp"sv,  IMAGE_BMP},
    {".ico"sv,  IMAGE_ICO},
    {".tiff"sv, IMAGE_TIFF},
    {".tif"sv,  IMAGE_TIFF},
    {".svg"sv,  IMAGE_SVG},
    {".svgz"sv, IMAGE_SVG},
    {".mp3"sv,  AUDIO_MP3},
};

const std::unordered_map<http::verb, std::string_view> Methods::method_to_str =
{
    {http::verb::get,  GET},
    {http::verb::head, HEAD},
    {http::verb::post, POST},
};

StringResponse MakeStringResponse(http::status status, std::string_view body, const http_request::RequestData& req_data, std::string_view content_type) {
    StringResponse response(status, req_data.http_version);
    response.set(http::field::content_type, content_type);
    response.set(http::field::cache_control, ConstantsResponse::NO_CACHE);

    if (req_data.method == http::verb::head){
        response.content_length(body.size());
    }else if (body.empty() && content_type == ContentType::APPLICATION_JSON) {
        response.body() = ConstantsResponse::EMPTY_JSON;
        response.content_length(ConstantsResponse::EMPTY_JSON.size());
    } else {
        response.body() = body;
        response.content_length(body.size());
    }
    response.keep_alive(req_data.keep_alive);
    return response;
}

}