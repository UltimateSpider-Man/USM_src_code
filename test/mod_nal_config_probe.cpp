#include "../src/mod_nal_config.h"
#include <iostream>
#include <stdexcept>

using modmesh::nalconfig::Config;
using modmesh::nalconfig::Kind;
using modmesh::aliases::Dictionary;
using modmesh::aliases::gameHash;

static void require(bool value, const std::string &message)
{
    if (!value) throw std::runtime_error(message);
}

static std::string row(const std::string &name)
{
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << gameHash(name)
        << '\t' << name << '\n';
    return out.str();
}

int main(int argc, char **argv)
{
    try {
        Dictionary dictionary;
        std::string why;
        require(dictionary.parse(row("VENOM") + row("USM_BLACKSUIT") + row("ULTIMATE_SPIDERMAN")
                    + row("walk cycle") + row("RUN") + row("aa"), &why), why);
        Config config;
        const std::string valid = "\xEF\xBB\xBF# NAL mapping data\r\n"
            "[ SkEleTons ]\r\n0xe9216c56 = \"C:\\Models With Spaces\\Original.PcSkEl\" ; source\r\n"
            "[ANIMATIONS]\n vEnOm = ..\\banks\\My Bank.PCANIM\n"
            "[Clips]\nULTIMATE_SPIDERMAN / \"walk cycle\" = \"\\\\server\\share\\Anim #1;take.PCANIM\" :: run\n"
            "ULTIMATE_SPIDERMAN / 0xdeadbeef = D:\\banks\\clip.PCANIM :: 0xcafebabe\n"
            "\"c:/usm/data/bank\" / clip_name = /tmp/source.PCANIM :: donor_clip\n";
        require(config.parse(valid, dictionary, &why), why);
        require(config.size() == 5, "valid mappings lost");
        const auto &m = config.mappings();
        require(m[0].kind == Kind::Skeleton && m[0].bank.hash == 0xe9216c56u
                    && m[0].bank.name == "USM_BLACKSUIT"
                    && m[0].sourceFile == "C:\\Models With Spaces\\Original.PcSkEl",
                "skeleton literal hash or absolute path changed");
        require(m[1].kind == Kind::Animation && m[1].bank.hash == gameHash("VENOM")
                    && m[1].sourceFile == "..\\banks\\My Bank.PCANIM",
                "animation bank alias/relative source path changed");
        require(m[2].kind == Kind::Clip && m[2].targetClip.hash == gameHash("walk cycle")
                    && m[2].sourceClip.name == "RUN"
                    && m[2].sourceFile == "\\\\server\\share\\Anim #1;take.PCANIM",
                "quoted UNC path, comment characters or clip identity lost");
        require(m[3].targetClip.hash == 0xdeadbeefu && m[3].sourceClip.hash == 0xcafebabeu
                    && m[3].sourceFile == "D:\\banks\\clip.PCANIM",
                "unknown literal clip hashes or Windows drive colon misparsed");
        require(m[4].bank.name == "c:/usm/data/bank" && m[4].sourceFile == "/tmp/source.PCANIM",
                "quoted resource path or absolute source path rejected");
        size_t rejected = 0;
        const auto reject = [&](const std::string &bad) {
            require(!config.parse(bad, dictionary, &why), "malformed config accepted: " + bad);
            require(config.size() == 5 && config.mappings()[0].sourceFile == "C:\\Models With Spaces\\Original.PcSkEl",
                    "failed parse destroyed previous configuration");
            require(!why.empty(), "failed parse lacks a diagnostic");
            ++rejected;
        };
        for (const char *bad : {
            "VENOM = source.PCANIM", "[]", "[unknown]", "[clips] trailing",
            "[animations]\nVENOM source.PCANIM", "[animations]\n = source.PCANIM",
            "[animations]\nVENOM =", "[animations]\nVENOM = \"unterminated.PCANIM",
            "[animations]\nVENOM = bad\"quoted\".PCANIM",
            "[skeletons]\nVENOM = source.PCANIM", "[animations]\nVENOM = source.PCSKEL",
            "[animations]\nVENOM = source.PCANIM:stream", "[animations]\nVENOM = .PCANIM",
            "[clips]\nVENOM = source.PCANIM :: RUN", "[clips]\nVENOM / WALK = source.PCANIM",
            "[clips]\nVENOM / WALK / EXTRA = source.PCANIM :: RUN",
            "[clips]\nVENOM / WALK = source.PCANIM :: RUN :: EXTRA",
            "[clips]\nVENOM / = source.PCANIM :: RUN", "[clips]\nVENOM / WALK = source.PCANIM ::",
            "[clips]\nVENOM / WALK = source.PCSKEL :: RUN",
            "[animations]\nVENOM = a.PCANIM\n0x08909065 = b.PCANIM",
            "[skeletons]\nVENOM = a.PCSKEL\nvenom = b.PCSKEL",
            "[clips]\nVENOM / WALK = a.PCANIM :: RUN\nvenom / walk = b.PCANIM :: RUN",
            "[animations]\nVENOM = a.PCANIM\n[clips]\nvenom / WALK = b.PCANIM :: RUN",
            "[clips]\nVENOM / WALK = b.PCANIM :: RUN\n[animations]\n0x08909065 = a.PCANIM",
            "[animations]\n0x100000000 = source.PCANIM", "[skeletons]\nb@ = source.PCSKEL",
            "[clips]\nVENOM / b@ = source.PCANIM :: RUN",
            "[clips]\nVENOM / WALK = source.PCANIM :: b@"
        }) reject(bad);
        reject(std::string("[animations]\nVENOM = x.PCANIM\n#") + '\0' + "hidden");
        reject("[animations]\nVENOM = x.PCANIM\rhidden");
        reject("[animations]\nVENOM = \"tab\tname.PCANIM\"");
        reject(std::string("[animations]\nVENOM = x") + char(127) + ".PCANIM");
        reject(std::string("[animations]\nVENOM = ") + std::string(Config::maxSourceBytes, 'x') + ".PCANIM");
        reject(std::string(Config::maxBytes + 1, '#'));
        std::string many = "[skeletons]\n";
        for (size_t i = 0; i <= Config::maxEntries; ++i)
            many += "custom_" + std::to_string(i) + " = x.PCSKEL\n";
        reject(many);
        require(!config.load("", dictionary, &why) && config.size() == 5,
                "failed file load replaced valid mappings");
        require(config.parse(many.substr(0, many.rfind("custom_")), dictionary, &why)
                    && config.size() == Config::maxEntries, "4096-entry boundary rejected");
        require(config.parse("[skeletons]\nVENOM = a.PCSKEL\n[animations]\nVENOM = a.PCANIM\n",
                              dictionary, &why) && config.size() == 2,
                "independent skeleton and bank mappings incorrectly conflict");
        require(config.parse("[animations]\nVENOM = \"folder\\a=b.PCANIM\"\n", dictionary, &why)
                    && config.mappings()[0].sourceFile == "folder\\a=b.PCANIM",
                "quoted assignment character in a source filename rejected");
        require(config.parse(std::string(Config::maxBytes, '#'), dictionary, &why)
                    && config.size() == 0, "exact byte boundary or empty-config clearing failed");
        if (argc == 2) {
            require(dictionary.load(argv[1], &why), why);
            require(dictionary.size() > 100000, "real dictionary incomplete");
            require(config.parse("[skeletons]\n0xe9216c56 = C:\\source\\USM_BLACKSUIT.PCSKEL\n"
                                 "[animations]\nVENOM = ..\\any source.PCANIM\n"
                                 "[clips]\n0x1189ab87 / 0xdeadbeef = C:\\source\\any.PCANIM :: 0xcafebabe\n",
                                 dictionary, &why) && config.size() == 3, why);
            std::cout << "PASS real dictionary: " << dictionary.size() << " names available to NAL resource/clip keys\n";
        } else require(argc == 1, "usage: mod_nal_config_probe [string_hash_dictionary.txt]");
        std::cout << "PASS NAL config paths, quoted delimiters, names/hashes, section and clip scope, atomicity, "
                  << rejected << " rejection cases and inclusive bounds\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
