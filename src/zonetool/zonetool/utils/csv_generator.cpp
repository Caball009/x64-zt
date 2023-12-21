#include <std_include.hpp>

#include "utils.hpp"
#include "csv_generator.hpp"

#include <utils/io.hpp>
#include <utils/string.hpp>

namespace csv_generator
{
	namespace
	{
		std::unordered_set<std::string> parse_create_fx_gsc(const std::string& data)
		{
			std::unordered_set<std::string> sounds;
			std::regex pattern("\\w+\\.v\\[\\s*\"soundalias\"\\s*\\]\\s*=\\s*\"([\\w\\s]+?)\"", std::regex_constants::icase);
			std::smatch matches;

			auto search_start = data.cbegin();
			while (std::regex_search(search_start, data.cend(), matches, pattern))
			{
				sounds.insert(matches[1].str());
				search_start = matches.suffix().first;
			}

			return sounds;
		}

		std::unordered_set<std::string> parse_fx_gsc(const std::string& data)
		{
			std::unordered_set<std::string> effects;
			std::regex pattern("loadfx\\s*\\(\\s*\"([^\"]+)\"\\s*\\);", std::regex_constants::icase);
			std::smatch matches;

			auto search_start = data.cbegin();
			while (std::regex_search(search_start, data.cend(), matches, pattern))
			{
				effects.insert(matches[1].str());
				search_start = matches.suffix().first;
			}

			return effects;
		}
	}

	void generate_map_csv(const std::string& map, const mapents::token_name_callback& get_token_name, bool is_sp)
	{
		const auto root_dir = "zonetool/" + map;
		if (!utils::io::directory_exists(root_dir))
		{
			ZONETOOL_ERROR("Zonetool map directory \"%s\" does not exist", map.data());
			return;
		}

		const auto map_prefix = map.starts_with("mp_")
			? "maps/mp"s
			: "maps"s;
		const auto map_prefix_full = root_dir + "/" + map_prefix + "/";
		const auto mapents_path = map_prefix_full + map + ".d3dbsp.ents";

		std::string mapents_data;
		if (!utils::io::read_file(mapents_path, &mapents_data))
		{
			ZONETOOL_ERROR("Missing mapents");
			return;
		}

		ZONETOOL_INFO("Generating CSV for map \"%s\"", map.data());

		std::string csv;

		const auto add_str = [&](const std::string& str)
		{
			csv.append(str);
		};

		const auto add_line = [&](const std::string& line)
		{
			add_str(line);
			add_str("\n");
		};

		const auto add_asset = [&](const std::string& type, const std::string& name)
		{
			ZONETOOL_INFO("Adding %s \"%s\"", type.data(), name.data());
			add_line(utils::string::va("%s,%s", type.data(), name.data()));
		};

		add_line("// Generated by x64-ZoneTool\n");

		if (!is_sp)
		{
			add_line("// netconststrings");
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "mdl"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "mat"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "rmb"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "veh"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "vfx"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "loc"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "snd"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "sbx"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "snl"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "shk"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "mnu"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "tag"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "hic"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "nps"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "mic"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "sel"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "wep"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "att"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "hnt"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "anm"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "fxt"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "acl"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "lui"));
			add_asset("netconststrings", utils::string::va("ncs_%s_level", "lsr"));
			add_line("");
		}

		ZONETOOL_INFO("Parsing mapents...");
		const auto mapents_list = mapents::parse(mapents_data, get_token_name);

		std::unordered_set<std::string> added_models;
		auto added_models_comment = false;
		for (const auto& ent : mapents_list.entities)
		{
			if (ent.get("classname") != "script_model")
			{
				continue;
			}

			const auto model = ent.get("model");
			if (model == "")
			{
				continue;
			}

			if (added_models.contains(model))
			{
				continue;
			}

			if (!added_models_comment)
			{
				added_models_comment = true;
				add_line("// models");
			}

			added_models.insert(model);
			add_asset("xmodel", model);
		}
		if (added_models.size() > 0)
		{
			add_line("");
		}

		const std::string create_fx_name = utils::string::va("maps/createfx/%s_fx.gsc", map.data());
		const std::string create_fx_sounds_name = utils::string::va("maps/createfx/%s_sound.gsc", map.data());

		const auto add_sounds = [&](const std::string& file)
		{
			const std::string create_fx_path = utils::string::va("%s/%s", root_dir.data(), file.data());

			std::string data;
			if (!utils::io::read_file(create_fx_path, &data))
			{
				return;
			}

			ZONETOOL_INFO("Parsing createfx gsc...");

			add_line("// sounds");
			const auto sounds = parse_create_fx_gsc(data);
			for (const auto& sound : sounds)
			{
				add_asset("sound", sound);
			}

			add_line("");
		};

		const std::string fx_name = utils::string::va("%s/%s_fx.gsc", map_prefix.data(), map.data());
		const auto add_effects = [&]
		{
			const std::string fx_path = utils::string::va("%s/%s", root_dir.data(), fx_name.data());

			std::string data;
			if (!utils::io::read_file(fx_path, &data))
			{
				return;
			}

			ZONETOOL_INFO("Parsing fx gsc...");

			add_line("// effects");

			const auto effects = parse_fx_gsc(data);
			for (const auto& effect : effects)
			{
				add_asset("fx", effect);
			}

			add_line("");
		};

		add_sounds(create_fx_name);
		add_sounds(create_fx_sounds_name);
		add_effects();

		const auto add_map_asset = [&](const std::string& type, const std::string& ext)
		{
			std::string name = utils::string::va("%s/%s.d3dbsp", map_prefix.data(), map.data());

			const std::string path = root_dir + "/" + name + ext;
			if (!utils::io::file_exists(path))
			{
				add_str("#");
			}
			add_asset(type, name);
		};

		const auto add_iterator = [&](const std::string& type, const std::string& folder,
			const std::string& extension, const std::string& comment, bool path = true)
		{
			auto added_comment = false;
			const auto folder_ = root_dir + "/" + folder;
			if (!utils::io::directory_exists(folder_))
			{
				return;
			}

			const auto files = utils::io::list_files(folder_);
			for (const auto& file : files)
			{
				if (!file.ends_with(extension))
				{
					continue;
				}

				if (!added_comment)
				{
					added_comment = true;
					add_line(comment);
				}

				if (!path)
				{
					const std::string name = file.substr(folder_.size(), file.size() - folder_.size() - extension.size());
					add_asset(type, name);
				}
				else
				{
					const std::string name = folder + file.substr(folder_.size());
					add_asset(type, name);
				}
			}

			if (added_comment)
			{
				add_line("");
			}
		};

		const auto add_if_exists = [&](const std::string& text, const std::string& path)
		{
			if (!utils::io::file_exists(root_dir + "/" + path))
			{
				return false;
			}

			add_line(text);
			return true;
		};

		{
			const std::string compass_name = utils::string::va("compass_map_%s", map.data());
			const std::string compass_path = utils::string::va("materials/%s.json", compass_name.data());
			add_if_exists(utils::string::va("// compass\nmaterial,compass_map_%s\n", map.data()), compass_path);
		}

		add_iterator("stringtable", "maps/createart/", ".csv", "// lightsets");
		add_iterator("clut", "clut/", ".clut", "// color lookup tables", false);
		add_iterator("rawfile", "vision/", ".vision", "// visions");
		add_iterator("rawfile", "sun/", ".sun", "// sun");

		const auto add_gsc = [&](const std::string& path)
		{
			const std::string gsc_path = root_dir + "/" + path;
			if (!utils::io::file_exists(gsc_path))
			{
				add_str("#");
			}
			
			add_asset("rawfile", path);
		};

		const auto add_gsc_if_exists = [&](const std::string& path)
		{
			const std::string gsc_path = root_dir + "/" + path;
			if (!utils::io::file_exists(root_dir + "/" + path))
			{
				return false;
			}

			add_asset("rawfile", path);
			return true;
		};

		add_line("// gsc");
		add_gsc(utils::string::va("%s/%s.gsc", map_prefix.data(), map.data()));
		add_gsc(fx_name);
		add_gsc(create_fx_name);
		add_gsc_if_exists(create_fx_sounds_name);
		add_gsc_if_exists(utils::string::va("%s/%s_precache.gsc", map_prefix.data(), map.data()));
		add_gsc_if_exists(utils::string::va("%s/%s_lighting.gsc", map_prefix.data(), map.data()));
		add_gsc_if_exists(utils::string::va("%s/%s_aud.gsc", map_prefix.data(), map.data()));
		add_gsc(utils::string::va("maps/createart/%s_art.gsc", map.data()));
		add_gsc(utils::string::va("maps/createart/%s_fog.gsc", map.data()));
		add_gsc(utils::string::va("maps/createart/%s_fog_hdr.gsc", map.data()));
		add_line("");

		ZONETOOL_INFO("Adding map assets...");

		add_line("// map assets");
		add_map_asset("com_map", ".commap");
		add_map_asset("fx_map", ".fxmap");
		add_map_asset("gfx_map", ".gfxmap");
		add_map_asset("map_ents", ".ents");
		add_map_asset("glass_map", ".glassmap");
		add_map_asset("phys_worldmap", ".physmap");
		add_map_asset("aipaths", ".aipaths");
		add_map_asset(is_sp ? "col_map_sp" : "col_map_mp", ".colmap");
		add_line("");

		const auto csv_path = "zone_source/" + map + ".csv";

		ZONETOOL_INFO("CSV saved to %s", csv_path.data());
		utils::io::write_file(csv_path, csv, false);
	}
}
