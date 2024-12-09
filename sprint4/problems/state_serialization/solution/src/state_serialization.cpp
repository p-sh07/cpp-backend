#include "state_serialization.h"

serialization::DogRepr::DogRepr(const model::Dog& dog): id_(dog.GetId())
    , pos_(dog.GetPos())
    , width_(dog.GetWidth())
    , speed_(dog.GetSpeed())
    , direction_(dog.GetDirection())
    , tag_(dog.GetTag())
    , bag_capacity_(dog.GetBagCap())
    , score_(dog.GetScore())
    , bag_content_(dog.GetBag()) {
}

model::Dog serialization::DogRepr::Restore() const {
    model::Dog dog{id_, pos_, width_, tag_, bag_capacity_};
    dog.SetSpeed(speed_);
    dog.SetDirection(direction_);
    dog.AddScore(score_);
    for (const auto& item : bag_content_) {
        if (!dog.TryCollectItem(item)) {
            throw std::runtime_error("Failed to put bag content");
        }
    }
    return dog;
}

serialization::LootItemRepr::LootItemRepr(const model::LootItem& item)
    : id(item.GetId())
    , type(item.GetType())
    , pos(item.GetPos()) {
}

serialization::SessionRepr::SessionRepr(const app::Session& session)
    : id_(session.GetId())
    , map_id_content_(*session.GetMapId()) {
    for(const auto& [_, dog] : session.GetDogs()) {
        dog_reprs_.emplace_back(DogRepr{dog});
    }
    for(const auto& item : session.GetLootItems()) {
        loot_item_reprs_.emplace_back(LootItemRepr{*item});
    }
}

app::Session serialization::SessionRepr::Restore(const app::GamePtr& game) const {
    const model::Map::Id map_id{map_id_content_};
    app::Session session(id_, game->FindMap(map_id), game->GetSettings());

    //Restore dogs and loot items
    for(const auto& dog : dog_reprs_) {
        session.AddDog(std::move(dog.Restore()));
    }
    for(const auto& item : loot_item_reprs_) {
        session.AddLootItem(item.id, item.type, item.pos);
    }
    return session;
}

app::Session::Id serialization::SessionRepr::GetId() const {
    return id_;
}

serialization::PlayerRepr::PlayerRepr(const app::Player& player, app::Token token): id_(player.GetId())
    , session_id_(player.GetSession()->GetId())
    , dog_id_(player.GetDog()->GetId())
    , token_content_(std::move(*token)) {
}

app::Player::Id serialization::PlayerRepr::GetId() const {
    return id_;
}

app::Session::Id serialization::PlayerRepr::GetSessionId() const {
    return session_id_;
}

model::Dog::Id serialization::PlayerRepr::GetDogId() const {
    return dog_id_;
}

app::Token serialization::PlayerRepr::GetToken() const {
    return app::Token{token_content_};
}

serialization::PsmRepr::PsmRepr(const app::PlayerSessionManager& psm) {
    for(const auto& [_, session] : psm.GetAllSessions()) {
        session_reprs_.emplace_back(session);
    }

    for(const auto& [_, player] : psm.GetAllPlayers()) {
        if(auto token = psm.GetToken(player.GetId()); token) {
            player_reprs_.emplace_back(player, *token);
        } else {
            //DEBUG
            std::cerr << "invalid pl.token found when saving game state" << std::endl;
        }
    }
}

app::PlayerSessionManager serialization::PsmRepr::Restore(const app::GamePtr& game) const {
    //Restores loot items and dogs inside session
    size_t next_session_id = 0u;
    size_t next_p = 0u;
    app::PlayerSessionManager::Sessions restored_sessions;
    for(const auto sess_repr : session_reprs_) {
        auto id = sess_repr.GetId();
        restored_sessions.emplace(id, std::move(sess_repr.Restore(game)));
    }
    //Restore players in the player manager
    app::PlayerSessionManager psm(game, std::move(restored_sessions));
    for(const auto plr : player_reprs_) {
        psm.RestorePlayer(plr.GetId(), plr.GetDogId(), plr.GetSessionId(), std::move(plr.GetToken()));
    }
    return psm;
}

serialization::StateSerializer::StateSerializer(fs::path save_file, bool enable_periodic_backup, int64_t save_period)
    : save_file_(save_file.filename())
    , save_dir_(save_file.parent_path())
    , enable_periodic_backup_(enable_periodic_backup)
    , save_period_(save_period) {
}

void serialization::StateSerializer::OnTick(model::TimeMs delta_t, const app::PlayerSessionManager& psm) {
    time_since_last_save_ += delta_t;

    //Do not save On tick if save file or save_period are nullopt
    if(!enable_periodic_backup_ || time_since_last_save_ < save_period_) {
        time_since_last_save_ += delta_t;
        return;
    }

    //Otherwise, need to save game state
    time_since_last_save_ = model::TimeMs{0u};
    SaveGameState(psm);
}

void serialization::StateSerializer::SaveGameState(const app::PlayerSessionManager& psm) const {
    //Create directories if they do not exist
    fs::create_directories(save_dir_);

    fs::path temp_full_path = save_dir_ / save_temp_;
    std::ofstream temp{temp_full_path, std::ios_base::ate};

    if(!temp) {
        throw std::runtime_error("unable to open temp save file");
    }

    arch::text_oarchive out{temp};

    PsmRepr player_manager_state{psm};
    out << player_manager_state;

    fs::rename(temp_full_path, save_dir_ / save_file_);
}

app::PlayerSessionManager serialization::StateSerializer::Restore(app::GamePtr game) const {
    if(!fs::exists(save_dir_ / save_file_)) {
        return std::move(app::PlayerSessionManager{game});
    }

    std::ifstream saved_state{save_dir_ / save_file_};

    if(!saved_state) {
        throw std::runtime_error("unable to open saved state file");
    }

    arch::text_iarchive in{saved_state};

    PsmRepr psm_repr{};
    in >> psm_repr;

    return std::move(psm_repr.Restore(game));
}
