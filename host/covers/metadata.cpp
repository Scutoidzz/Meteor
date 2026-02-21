#TODO: implement the metadata parsing to the HTML

#include <taglib/fileref.h>
#include <taglib/tag.h>

int printAudioMeta(const std::string& path) {
    TagLib::FileRef f(path.c_str());
    if (f.isNull() || !f.tag()) return;

    TagLib::Tag* tag = f.tag();
    std::cout << "Title: "  << tag->title().toCString(true)  << "\n";
    std::cout << "Artist: " << tag->artist().toCString(true) << "\n";
    std::cout << "Album: "  << tag->album().toCString(true)  << "\n";
    std::cout << "Year: "   << tag->year()                   << "\n";

    std::string s = std::string("MeteorSong;") + tag->title().toCString(true) + tag->artist().toCString(true) + tag->album().toCString(true);
}
