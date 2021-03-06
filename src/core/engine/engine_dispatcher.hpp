#pragma once

#include "core/engine/engine.hpp"
#include "core/engine/nullengine.hpp"
#include "core/ui/icons.hpp"
#include "util/flat_map.hpp"
#include "util/spin_lock.hpp"
#include "util/variant_w_base.hpp"

namespace otto::core::engine {

  struct EngineSelectorScreen;

  struct EngineSelectorData {
    util::string_ref name;
    ui::Icon icon;
    std::vector<std::string> presets;
  };

  using SelectedEngine = itc::PropTypes<struct selected_engine_tag, int>;
  using SelectedPreset = itc::PropTypes<struct selected_preset_tag, int>;
  struct Actions {
    using publish_engine_data = itc::Action<struct publish_engine_data_tag, EngineSelectorData>;
    using make_new_preset = itc::Action<struct make_new_preset_tag, std::string>;
  };

  /// Owns engines of type `ET`, and dispatches to a selected one of them
  template<EngineType ET, typename... Engines>
  struct EngineDispatcher : input::InputHandler {
    using Sender = services::UISender<EngineSelectorScreen>;

    constexpr static std::array<util::string_ref, sizeof...(Engines)> engine_names = {{Engines::name...}};
    constexpr static bool has_off_engine = std::is_same_v<meta::head_t<meta::list<Engines...>>, OffEngine<ET>>;

    struct Props {
      template<typename Tag, typename Type, typename... Mixins>
      using Prop = typename Sender::template Prop<Tag, Type, Mixins...>;

      Sender sender;
      SelectedEngine::Prop<Sender> selected_engine_idx = {sender, 0, props::limits(0, sizeof...(Engines) - 1)};
      SelectedPreset::Prop<Sender> selected_preset_idx = {sender, 0, props::limits(0, 12)};
    };

    EngineDispatcher() noexcept;

    ui::ScreenAndInput selector_screen() noexcept;
    ui::ScreenAndInput engine_screen() noexcept;
    ITypedEngine<ET>& current();
    ITypedEngine<ET>* operator->();

    template<int N>
    auto process(audio::ProcessData<N> data) noexcept;

    void encoder(input::EncoderEvent) override;
    bool keypress(input::Key) override;

    void action(SelectedEngine::action, int v)
    {
      props.selected_engine_idx = v;
    }

    void action(SelectedPreset::action, int v)
    {
      props.selected_preset_idx = v;
    }

    void action(Actions::make_new_preset, std::string name);

    void from_json(const nlohmann::json&);
    nlohmann::json to_json() const;

  private:
    void send_presets_for(util::string_ref engine_name);
    void update_max_preset_idx();
    void save_engine_state();

    std::atomic<bool> engine_is_constructed_ = true;
    util::variant_w_base<ITypedEngine<ET>, Engines...> current_engine_ = std::in_place_index_t<0>();
    std::unique_ptr<EngineSelectorScreen> screen_;
    util::flat_map<std::string, nlohmann::json> engine_states_;
    Props props = {{*screen_}};
  };

} // namespace otto::core::engine

#include "engine_dispatcher.inl"
#include "engine_selector_screen.hpp"
