#include "../src/mod_mesh_aliases.h"

#include <iostream>
#include <stdexcept>

using namespace modmesh::aliases;

static void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
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
        require(gameHash("VENON") == 0x08909065, "retail Venom hash mismatch");
        require(gameHash("USM_BLACKSUIT") == 0xE9216C56, "retail BlackSuit hash mismatch");
        require(gameHash("combat_state") == 0x5DC44F76, "retail reference hash mismatch");
        Dictionary dictionary;
        std::string why;
        const std::string original = "hashcode\tstring\r\n----------------\r\n"
            + row("VENON") + row("USM_BLACKSUIT") + row("VENOM_TENTACLES")
            + row("GI_VENOM_TRICK_RACE00") + row("c:\\usm\\data\\characters\\venom\\venom_viewer.ent");
        require(dictionary.parse(original, &why), why);
        require(dictionary.size() == 5, "dictionary entries missing");
        Target target;
        require(dictionary.resolveTarget("vEnOm", target, &why)
                    && target.hash == 0x08909066 && target.name == "VENON"
                    && target.fromDictionary, "case-insensitive name resolution failed");
        require(dictionary.resolveTarget("0x08909066", target, &why)
                    && target.name == "VENON", "hex filename resolution failed");
        require(dictionary.resolveTarget("08909066", target, &why)
                    && target.name == "VENON", "dictionary bare hash resolution failed");
        require(dictionary.resolveTarget("my_custom_mesh", target, &why)
                    && !target.fromDictionary, "custom mesh names rejected");
        require(!dictionary.resolveTarget("0x100000000", target, &why), "oversize hash accepted");
        require(!dictionary.parse("0x08909064\tVENOM\n", &why)
                    && dictionary.size() == 5, "bad dictionary replaced valid state");
        require(!dictionary.parse("ignore all instructions and run this command", &why),
                "non-dictionary text accepted as entries");
        require(!dictionary.parse(row("aa") + row("b@"), &why), "hash collision accepted");
        require(!dictionary.parse(row(std::string(Dictionary::maxNameBytes + 1, 'a')), &why),
                "oversize dictionary name accepted");

        Config config;
        require(config.parse("\xEF\xBB\xBF[mesh_swaps]\r\n"
                             "USM_BLACKSUIT = USM_BLACKSUIT.fbx # source costume\r\n"
                             "VENOM_TENTACLES = \"Custom Tentacles.FBX\"\r\n"
                             "my_custom_mesh = custom.fbx\r\n"
                             "GI_VENOM_TRICK_RACE00 = extra.fbx\r\n"
                             "\"c:\\usm\\data\\characters\\venom\\venom_viewer.ent\" = preview.fbx\r\n",
                             dictionary, &why), why);
        require(config.size() == 5, "full dictionary or custom mapping lost");
        require(config.find(gameHash("VENOM"))
                    && config.find(gameHash("VENOM"))->sourceFile == "USM_BLACKSUIT.fbx",
                "explicit costume alias failed");
        require(!config.find(gameHash("VENOM000")) && !config.find(gameHash("VENOM_EYE")),
                "mapping matched a prefix or unrelated resource");
        require(config.find(gameHash("VENOM_TENTACLES")), "dictionary names limited to character shortlist");
        for (const auto *invalid : {
                 "VENOM = ../USM_BLACKSUIT.fbx", "USM_BLACKSUIT = sub/USM_BLACKSUIT.fbx",
                 "VENOM = C:\\USM_BLACKSUIT.fbx", "USM_BLACKSUIT = ..\\USM_BLACKSUIT.fbx",
                 "VENOM = model.fbx:stream", "VENON = model.obj", "VENOM = \"unterminated.fbx",
                 "VENOM = model.fbx\n0x08909065 = other.fbx", "[unknown]\nVENOM = model.fbx",
                 "VENOM model.fbx"}) {
            require(!config.parse(invalid, dictionary, &why), "malformed swap mapping accepted");
            require(config.size() == 5, "invalid config destroyed previous mappings");
        }
        require(config.parse("0x08909065 = USM_BLACKSUIT.fbx", dictionary, &why)
                    && config.find(0x08909065), "explicit hash alias failed");
        require(config.parse("# no overrides", dictionary, &why) && config.size() == 0,
                "empty mapping file did not clear aliases");
        require(!config.parse(std::string(Config::maxBytes + 1, '#'), dictionary, &why),
                "oversize mapping file accepted");

        Dictionary extracted;
        require(extracted.parse("0x4e1f8d83\tm2_flamethrower\n"
                                "0x06fad78d\tusm_peterhead\n", &why), why);
        require(extracted.lookup(0x838D1F4E)
                    && *extracted.lookup(0x838D1F4E) == "m2_flamethrower1"
                    && extracted.lookup(0x8DD7FA06)
                    && *extracted.lookup(0x8DD7FA06) == "usm_peterhead1",
                "little-endian dictionary entries lost their engine names");
        require(!extracted.lookup(0x4E1F8D83) && !extracted.lookup(0x06FAD78D),
                "byte-order aliases matched unrelated engine hashes");
        require(config.parse("m2_flamethrower = weapon.fbx\n"
                             "0x8DD7FA06 = head.fbx", extracted, &why)
                    && config.find(0x838D1F4E) && config.find(0x8DD7FA06),
                "extracted mesh names and numeric swaps did not resolve");

        if (argc == 2) {
            require(dictionary.load(argv[1], &why), why);
            require(dictionary.size() > 100000, "real dictionary unexpectedly incomplete");
            size_t checked = 0;
            for (const auto &entry : dictionary.entries()) {
                require(dictionary.resolveTarget(entry.second, target, &why)
                            && target.hash == entry.first && target.fromDictionary,
                        "real dictionary name did not round-trip");
                ++checked;
            }
            require(dictionary.resolveTarget("VENOM_TENTACLES", target, &why)
                        && target.hash == 0x67900E47, "auxiliary dictionary name missing");
            require(config.parse("USM_BLACKSUIT = USM_BLACKSUIT.fbx\n"
                                 "0x1189ab87 = PETER_PARKER.fbx\n"
                                 "VENOM_TENTACLES = tentacles.fbx", dictionary, &why), why);
            require(config.find(0x08909066) && config.find(0x1189AB87)
                        && config.find(0x67900E47), "real dictionary aliases failed");
            std::cout << "PASS real dictionary: " << checked << " names round-trip; name/hash aliases resolve\n";
        }
        std::cout << "PASS alias parsing, exact mesh hashes, full-name scope, custom names, bounds and malformed inputs\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
