#include <iostream>
#include <uuid.h>


/**
 * TODO:
 * For movement speed, add a relative coordinate to block. For example x_block and the value should be normalized and in float so 1.0 from the start and -1.0 to the end.
 * 0.0 is center
 */


inline void hash_combine(std::size_t& seed) {
}

template<typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    hash_combine(seed, rest...);
}

using BlockId = uuids::uuid;
using EntityId = uuids::uuid;


/**
 * Represents the coordinate within a town space. So each value is a block.
 */
struct TownCoordinate {
    TownCoordinate()
            : left{-1}, top{-1} {

    }

    TownCoordinate(int left_, int top_)
            : left{left_}, top{top_} {

    }

    int left;
    int top;

    bool operator==(const TownCoordinate& other) const {
        return this->left == other.left && this->top == other.top;
    }
};

/**
 * Represents the coordinate within a block space. So each value is a percentage within a block.
 */
struct BlockCoordinate {
    BlockCoordinate()
            : left{0.0F}, top{0.0F} {

    }

    BlockCoordinate(float left_, float top_)
            : left{left_}, top{top_} {

    }

    float left;
    float top;
};

namespace std {
    template<>
    struct hash<TownCoordinate> {
        inline size_t operator()(const TownCoordinate& coordinate) const {
            size_t value = 0;
            hash_combine(value, coordinate.left, coordinate.top);

            return value;
        }
    };
}

struct Town {
    std::unordered_map<TownCoordinate, BlockId> blocks;
};

struct BlockData {
    enum class Type {
        GRASS,
        ROAD,
        BUILDING
    };

    BlockData()
            : id{}, type{Type::GRASS}, entities{} {

    }

    BlockData(BlockId id_, Type type_)
            : id{id_}, type{type_}, entities{} {

    }

    BlockId id;
    Type type;
    std::vector<EntityId> entities;
};

struct BlockStorage {
    std::unordered_map<BlockId, BlockData> blocks;
};

struct MovementComponent {
    MovementComponent()
            : speed{0.0F} {

    }

    explicit MovementComponent(float speed_)
            : speed{speed_} {

    }

    float speed;
};

struct EntityData {
    static constexpr const float MAX_MOVEMENT_SPEED = 100.0F;

    enum class Type {
        NONE,
        HERO,
        TREE
    };

    EntityData()
            : id{}, type{Type::NONE}, town_coord{}, block_coord{} {

    }

    EntityData(EntityId id_, Type type_, TownCoordinate town_coord_)
            : id{id_}, type{type_}, town_coord{town_coord_}, block_coord{0.0F, 0.0F} {

    }

    EntityId id;
    Type type;
    TownCoordinate town_coord;
    BlockCoordinate block_coord;

    std::optional<MovementComponent> movement;
};

struct EntityStorage {
    std::unordered_map<EntityId, EntityData> entities;
};

struct MovementData {
    TownCoordinate old_town_coord;
    BlockCoordinate old_block_coord;
    TownCoordinate new_town_coord;
    BlockCoordinate new_block_coord;
};

// TODO: Temp code. This is doing too much. Storage responsibility should be a different function than adding it to the town.
void add_block(uuids::uuid_random_generator& uuid_rng, BlockStorage& storage, Town& town, TownCoordinate coord, BlockData::Type block_type) {
    BlockId id = uuid_rng();
    storage.blocks.emplace(std::make_pair(id, BlockData{id, block_type}));
    town.blocks.emplace(std::make_pair(coord, id)); // NOTE: Linking the coordinate with the associated block data.
}

BlockData& get_block(BlockStorage& storage, const Town& town, TownCoordinate coord) {
    return storage.blocks[town.blocks.at(coord)];
}

void add_entity_to_block(BlockStorage& storage, const Town& town, TownCoordinate town_coord, EntityId entity_id) {
    get_block(storage, town, town_coord).entities.push_back(entity_id);
}

void move_entity_to_new_block(BlockStorage& storage, const Town& town, TownCoordinate new_town_coord, const EntityData& entity_data) {
    BlockData current_block = get_block(storage, town, entity_data.town_coord);
    current_block.entities.erase(std::find(current_block.entities.begin(), current_block.entities.end(), entity_data.id));

    add_entity_to_block(storage, town, new_town_coord, entity_data.id);
}

// TODO: Temp code. This is doing too much. Same todo as add_block.
// TODO: This need tons of dependencies. We should figure out how to reduce the dependencies.
EntityData& add_entity(
        uuids::uuid_random_generator& uuid_rng, BlockStorage& block_storage, Town& town, EntityStorage& entity_storage, EntityData::Type type, TownCoordinate town_coord
) {
    EntityId id = uuid_rng();
    entity_storage.entities.emplace(std::make_pair(id, EntityData{id, type, town_coord}));
    get_block(block_storage, town, town_coord).entities.push_back(id);

    return entity_storage.entities[id];
}

// TODO: Consider direction here in the future so we can have a velocity
MovementData do_movement(TownCoordinate current_town_coord, BlockCoordinate current_block_coord, const MovementComponent& movement) {
    TownCoordinate new_town_coord = current_town_coord;
    BlockCoordinate new_block_coord = current_block_coord;

    float amount_to_move = movement.speed / EntityData::MAX_MOVEMENT_SPEED;
    float remainder = 1.0F - (current_block_coord.top + amount_to_move);

    if (remainder < 0.0F) {
        ++new_town_coord.top;
        new_block_coord.top = -1.0F + abs(remainder);
    } else {
        new_block_coord.top = current_block_coord.top + amount_to_move;
    }

    return MovementData{current_town_coord, current_block_coord, new_town_coord, new_block_coord};
}

void advance_move(BlockStorage& block_storage, Town& town, EntityData& entity_data, const MovementData& movement_data) {
    move_entity_to_new_block(block_storage, town, movement_data.new_town_coord, entity_data);

    entity_data.town_coord = movement_data.new_town_coord;
    entity_data.block_coord = movement_data.new_block_coord;
}

std::ostream& operator<<(std::ostream& stream, TownCoordinate coord) {
    stream << "[" << coord.left << ", " << coord.top << "]";

    return stream;
}

std::ostream& operator<<(std::ostream& stream, BlockCoordinate coord) {
    stream << "[" << coord.left << ", " << coord.top << "]";

    return stream;
}

int main() {
    std::mt19937 rng;
    auto uuid_rng = uuids::uuid_random_generator(rng);
    Town town;
    BlockStorage block_storage;
    add_block(uuid_rng, block_storage, town, TownCoordinate{0, 0}, BlockData::Type::GRASS);
    add_block(uuid_rng, block_storage, town, TownCoordinate{0, 1}, BlockData::Type::GRASS);
    add_block(uuid_rng, block_storage, town, TownCoordinate{0, 2}, BlockData::Type::GRASS);
    add_block(uuid_rng, block_storage, town, TownCoordinate{0, 3}, BlockData::Type::ROAD);

    EntityStorage entity_storage;
    EntityData& player = add_entity(uuid_rng, block_storage, town, entity_storage, EntityData::Type::HERO, TownCoordinate{0, 0});
    player.movement.emplace(20.0F);
    add_entity(uuid_rng, block_storage, town, entity_storage, EntityData::Type::TREE, TownCoordinate{0, 1});

    std::string input;

    do {
        std::cout << "Type your input: \n";
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        std::transform(input.begin(), input.end(), input.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (input == "walk") {
            MovementData movement_data = do_movement(player.town_coord, player.block_coord, *player.movement);
            advance_move(block_storage, town, player, movement_data);
            std::cout << "You walked forward.\n";
            std::cout << "Town Coordinate is " << player.town_coord << '\n';
            std::cout << "Block Coordinate is " << player.block_coord << '\n';
        } else if (input == "info") {
            std::cout << "You are standing at the block: " << (int) get_block(block_storage, town, player.town_coord).type << '\n';
            std::cout << "List of entities at the block: \n";

            // TODO: Need a faster way to get a list of entities in a block
            for (EntityId id: get_block(block_storage, town, player.town_coord).entities) {
                const EntityData& entity_data = entity_storage.entities[id];
                std::cout << "Entity: " << (int) entity_data.type << '\n';
            }
        }
    }
    while (!input.empty());

    return 0;
}
