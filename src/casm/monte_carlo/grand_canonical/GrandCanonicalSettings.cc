#include "casm/monte_carlo/grand_canonical/GrandCanonicalSettings.hh"

#include "casm/app/ProjectSettings.hh"
#include "casm/casm_io/container/json_io.hh"
#include "casm/enumerator/io/json/DoFSpace.hh"
#include "casm/monte_carlo/grand_canonical/GrandCanonicalConditions.hh"
#include "casm/monte_carlo/grand_canonical/GrandCanonicalIO.hh"

namespace CASM {
namespace Monte {

namespace {
std::string _help() {
  std::string s =
      "For GrandCanonicalConditions, expect a JSON object of form:\n"
      "  {\n"
      "    \"param_chem_pot\": {\n"
      "      \"a\" : -1.0,\n"
      "      ...\n"
      "    },\n"
      "    \"temperature\" : 350.0,\n"
      "    \"tolerance\" : 0.001\n"
      "  }\n";
  return s;
}
}  // namespace

/// \brief Construct EquilibriumMonteSettings by reading a settings JSON file
GrandCanonicalSettings::GrandCanonicalSettings(const PrimClex &_primclex,
                                               const fs::path &read_path)
    : EquilibriumMonteSettings(_primclex, read_path) {
  if (!_primclex.has_composition_axes()) {
    throw std::runtime_error("No composition axes selected.");
  }
}

// --- GrandCanonicalConditions settings ---------------------

/// \brief Expects initial_conditions
GrandCanonicalConditions GrandCanonicalSettings::initial_conditions(
    GrandCanonical const &mc) const {
  if (drive_mode() == Monte::DRIVE_MODE::INCREMENTAL) {
    return _conditions("initial_conditions", mc);
  } else if (drive_mode() == Monte::DRIVE_MODE::CUSTOM) {
    return custom_conditions(mc)[0];
  } else {
    throw std::runtime_error("ERROR: Invalid drive mode.");
  }
}

/// \brief Expects final_conditions
GrandCanonicalConditions GrandCanonicalSettings::final_conditions(
    GrandCanonical const &mc) const {
  return _conditions("final_conditions", mc);
}

/// \brief Expects incremental_conditions
GrandCanonicalConditions GrandCanonicalSettings::incremental_conditions(
    GrandCanonical const &mc) const {
  return _conditions("incremental_conditions", mc);
}

/// \brief Expects incremental_conditions
std::vector<GrandCanonicalConditions> GrandCanonicalSettings::custom_conditions(
    GrandCanonical const &mc) const {
  std::string level1 = "driver";
  std::string level2 = "custom_conditions";

  try {
    std::vector<GrandCanonicalConditions> cond;
    const jsonParser &json = (*this)[level1][level2];
    for (auto it = json.begin(); it != json.end(); ++it) {
      cond.push_back(_conditions(*it, mc));
    }
    return cond;
  } catch (std::runtime_error &e) {
    Log &err_log = CASM::err_log();
    err_log.error<Log::standard>("Reading Monte Carlo settings");
    err_log << "Tried to read an array of GrandCanonicalConditions from [\""
            << level1 << "\"][\"" << level2 << "\"]" << std::endl;
    err_log << _help() << std::endl;
    throw e;
  }
}

// --- Project settings ---------------------

/// \brief Get formation energy cluster expansion
ClexDescription GrandCanonicalSettings::formation_energy(
    const PrimClex &primclex) const {
  const ProjectSettings &set = primclex.settings();
  std::string level1 = "model";
  // deprecated
  if (_is_setting(level1, "clex")) {
    // expect "clex" is "formation_energy"

    std::vector<std::string> var{"clex", "calctype", "ref", "bset", "eci"};
    std::vector<std::string> help{
        "string\n  Names the cluster expansion to be used.\n",
        "string\n  Names the calctype to be used.\n",
        "string\n  Names the reference to be used.\n",
        "string\n  Names the basis set to be used.\n",
        "string\n  Names the ECI to be used.\n"};

    return ClexDescription(_get_setting<std::string>(level1, var[0], help[0]),
                           _get_setting<std::string>(level1, var[0], help[0]),
                           _get_setting<std::string>(level1, var[1], help[1]),
                           _get_setting<std::string>(level1, var[2], help[2]),
                           _get_setting<std::string>(level1, var[3], help[3]),
                           _get_setting<std::string>(level1, var[4], help[4]));
  }

  std::string help =
      "(string, default='formation_energy')\n"
      "  Names the formation_energy cluster expansion to be used.\n";

  std::string formation_energy = "formation_energy";
  if (_is_setting(level1, "formation_energy")) {
    formation_energy =
        _get_setting<std::string>(level1, "formation_energy", help);
  }

  if (!set.has_clex(formation_energy)) {
    Log &err_log = CASM::err_log();
    err_log.error<Log::standard>("Reading Monte Carlo settings");
    err_log << "Error reading [\"model\"][\"formation_energy\"]\n";
    err_log << "[\"model\"][\"formation_energy\"]: (string, optional, "
               "default='formation_energy')\n";
    err_log << "  Names the cluster expansion to be used for calculating "
               "formation_energy.\n";
    err_log << "No cluster expansion named: '" << formation_energy
            << "' exists.\n";
  }
  return set.clex(formation_energy);
}

/// \brief Make order parameter calculator
std::shared_ptr<OrderParameter> GrandCanonicalSettings::make_order_parameter(
    const PrimClex &primclex) const {
  if (m_order_parameter_checked) {
    return m_order_parameter;
  }
  if (_is_setting("model", "order_parameter")) {
    std::unique_ptr<DoFSpace> value =
        (*this)["model"]["order_parameter"].make<DoFSpace>(
            primclex.shared_prim());
    m_order_parameter = std::make_shared<OrderParameter>(*value);
  }
  m_order_parameter_checked = true;
  return m_order_parameter;
}

/// \brief Make order parameter calculator
std::shared_ptr<std::vector<std::vector<int>>>
GrandCanonicalSettings::make_order_parameter_subspaces() const {
  std::vector<std::vector<int>> value;
  if (_is_setting("model", "order_parameter") &&
      _is_setting("model", "order_parameter", "subspaces")) {
    from_json(value, (*this)["model"]["order_parameter"]["subspaces"]);
  }
  return std::make_shared<std::vector<std::vector<int>>>(value);
}

// --- Sampler settings ---------------------

GrandCanonicalConditions GrandCanonicalSettings::_conditions(
    std::string name, GrandCanonical const &mc) const {
  std::string level1 = "driver";
  std::string level2 = name;
  try {
    return _conditions((*this)[level1][level2], mc);
  } catch (std::runtime_error &e) {
    Log &err_log = CASM::err_log();
    err_log.error<Log::standard>("Reading Monte Carlo settings");
    err_log << "Error reading: " << name << std::endl;
    err_log << "Tried to construct GrandCanonicalCondtions from [\"" << level1
            << "\"][\"" << level2 << "\"]" << std::endl;
    err_log << _help() << std::endl;
    throw e;
  }
}

GrandCanonicalConditions GrandCanonicalSettings::_conditions(
    const jsonParser &json, GrandCanonical const &mc) const {
  GrandCanonicalConditions result;
  from_json(result, primclex(), json, mc);
  return result;
}

}  // namespace Monte
}  // namespace CASM
