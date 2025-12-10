/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * Wallet Implementation - HD Wallet with BIP32/BIP39/BIP44
 */

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include "intcoin/blockchain.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <chrono>
#include <set>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/backup_engine.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>

using namespace intcoin;

namespace intcoin {
namespace wallet {

// ============================================================================
// BIP39 Word List (English - 2048 words)
// ============================================================================

static const std::vector<std::string> BIP39_WORDLIST = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract",
    "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid",
    "acoustic", "acquire", "across", "act", "action", "actor", "actress", "actual",
    "adapt", "add", "addict", "address", "adjust", "admit", "adult", "advance",
    "advice", "aerobic", "affair", "afford", "afraid", "again", "age", "agent",
    "agree", "ahead", "aim", "air", "airport", "aisle", "alarm", "album",
    "alcohol", "alert", "alien", "all", "alley", "allow", "almost", "alone",
    "alpha", "already", "also", "alter", "always", "amateur", "amazing", "among",
    "amount", "amused", "analyst", "anchor", "ancient", "anger", "angle", "angry",
    "animal", "ankle", "announce", "annual", "another", "answer", "antenna", "antique",
    "anxiety", "any", "apart", "apology", "appear", "apple", "approve", "april",
    "arch", "arctic", "area", "arena", "argue", "arm", "armed", "armor",
    "army", "around", "arrange", "arrest", "arrive", "arrow", "art", "artefact",
    "artist", "artwork", "ask", "aspect", "assault", "asset", "assist", "assume",
    "asthma", "athlete", "atom", "attack", "attend", "attitude", "attract", "auction",
    "audit", "august", "aunt", "author", "auto", "autumn", "average", "avocado",
    "avoid", "awake", "aware", "away", "awesome", "awful", "awkward", "axis",
    "baby", "bachelor", "bacon", "badge", "bag", "balance", "balcony", "ball",
    "bamboo", "banana", "banner", "bar", "barely", "bargain", "barrel", "base",
    "basic", "basket", "battle", "beach", "bean", "beauty", "because", "become",
    "beef", "before", "begin", "behave", "behind", "believe", "below", "belt",
    "bench", "benefit", "best", "betray", "better", "between", "beyond", "bicycle",
    "bid", "bike", "bind", "biology", "bird", "birth", "bitter", "black",
    "blade", "blame", "blanket", "blast", "bleak", "bless", "blind", "blood",
    "blossom", "blouse", "blue", "blur", "blush", "board", "boat", "body",
    "boil", "bomb", "bone", "bonus", "book", "boost", "border", "boring",
    "borrow", "boss", "bottom", "bounce", "box", "boy", "bracket", "brain",
    "brand", "brass", "brave", "bread", "breeze", "brick", "bridge", "brief",
    "bright", "bring", "brisk", "broccoli", "broken", "bronze", "broom", "brother",
    "brown", "brush", "bubble", "buddy", "budget", "buffalo", "build", "bulb",
    "bulk", "bullet", "bundle", "bunker", "burden", "burger", "burst", "bus",
    "business", "busy", "butter", "buyer", "buzz", "cabbage", "cabin", "cable",
    "cactus", "cage", "cake", "call", "calm", "camera", "camp", "can",
    "canal", "cancel", "candy", "cannon", "canoe", "canvas", "canyon", "capable",
    "capital", "captain", "car", "carbon", "card", "cargo", "carpet", "carry",
    "cart", "case", "cash", "casino", "castle", "casual", "cat", "catalog",
    "catch", "category", "cattle", "caught", "cause", "caution", "cave", "ceiling",
    "celery", "cement", "census", "century", "cereal", "certain", "chair", "chalk",
    "champion", "change", "chaos", "chapter", "charge", "chase", "chat", "cheap",
    "check", "cheese", "chef", "cherry", "chest", "chicken", "chief", "child",
    "chimney", "choice", "choose", "chronic", "chuckle", "chunk", "churn", "cigar",
    "cinnamon", "circle", "citizen", "city", "civil", "claim", "clap", "clarify",
    "claw", "clay", "clean", "clerk", "clever", "click", "client", "cliff",
    "climb", "clinic", "clip", "clock", "clog", "close", "cloth", "cloud",
    "clown", "club", "clump", "cluster", "clutch", "coach", "coast", "coconut",
    "code", "coffee", "coil", "coin", "collect", "color", "column", "combine",
    "come", "comfort", "comic", "common", "company", "concert", "conduct", "confirm",
    "congress", "connect", "consider", "control", "convince", "cook", "cool", "copper",
    "copy", "coral", "core", "corn", "correct", "cost", "cotton", "couch",
    "country", "couple", "course", "cousin", "cover", "coyote", "crack", "cradle",
    "craft", "cram", "crane", "crash", "crater", "crawl", "crazy", "cream",
    "credit", "creek", "crew", "cricket", "crime", "crisp", "critic", "crop",
    "cross", "crouch", "crowd", "crucial", "cruel", "cruise", "crumble", "crunch",
    "crush", "cry", "crystal", "cube", "culture", "cup", "cupboard", "curious",
    "current", "curtain", "curve", "cushion", "custom", "cute", "cycle", "dad",
    "damage", "damp", "dance", "danger", "daring", "dash", "daughter", "dawn",
    "day", "deal", "debate", "debris", "decade", "december", "decide", "decline",
    "decorate", "decrease", "deer", "defense", "define", "defy", "degree", "delay",
    "deliver", "demand", "demise", "denial", "dentist", "deny", "depart", "depend",
    "deposit", "depth", "deputy", "derive", "describe", "desert", "design", "desk",
    "despair", "destroy", "detail", "detect", "develop", "device", "devote", "diagram",
    "dial", "diamond", "diary", "dice", "diesel", "diet", "differ", "digital",
    "dignity", "dilemma", "dinner", "dinosaur", "direct", "dirt", "disagree", "discover",
    "disease", "dish", "dismiss", "disorder", "display", "distance", "divert", "divide",
    "divorce", "dizzy", "doctor", "document", "dog", "doll", "dolphin", "domain",
    "donate", "donkey", "donor", "door", "dose", "double", "dove", "draft",
    "dragon", "drama", "drastic", "draw", "dream", "dress", "drift", "drill",
    "drink", "drip", "drive", "drop", "drum", "dry", "duck", "dumb",
    "dune", "during", "dust", "dutch", "duty", "dwarf", "dynamic", "eager",
    "eagle", "early", "earn", "earth", "easily", "east", "easy", "echo",
    "ecology", "economy", "edge", "edit", "educate", "effort", "egg", "eight",
    "either", "elbow", "elder", "electric", "elegant", "element", "elephant", "elevator",
    "elite", "else", "embark", "embody", "embrace", "emerge", "emotion", "employ",
    "empower", "empty", "enable", "enact", "end", "endless", "endorse", "enemy",
    "energy", "enforce", "engage", "engine", "enhance", "enjoy", "enlist", "enough",
    "enrich", "enroll", "ensure", "enter", "entire", "entry", "envelope", "episode",
    "equal", "equip", "era", "erase", "erode", "erosion", "error", "erupt",
    "escape", "essay", "essence", "estate", "eternal", "ethics", "evidence", "evil",
    "evoke", "evolve", "exact", "example", "excess", "exchange", "excite", "exclude",
    "excuse", "execute", "exercise", "exhaust", "exhibit", "exile", "exist", "exit",
    "exotic", "expand", "expect", "expire", "explain", "expose", "express", "extend",
    "extra", "eye", "eyebrow", "fabric", "face", "faculty", "fade", "faint",
    "faith", "fall", "false", "fame", "family", "famous", "fan", "fancy",
    "fantasy", "farm", "fashion", "fat", "fatal", "father", "fatigue", "fault",
    "favorite", "feature", "february", "federal", "fee", "feed", "feel", "female",
    "fence", "festival", "fetch", "fever", "few", "fiber", "fiction", "field",
    "figure", "file", "film", "filter", "final", "find", "fine", "finger",
    "finish", "fire", "firm", "first", "fiscal", "fish", "fit", "fitness",
    "fix", "flag", "flame", "flash", "flat", "flavor", "flee", "flight",
    "flip", "float", "flock", "floor", "flower", "fluid", "flush", "fly",
    "foam", "focus", "fog", "foil", "fold", "follow", "food", "foot",
    "force", "forest", "forget", "fork", "fortune", "forum", "forward", "fossil",
    "foster", "found", "fox", "fragile", "frame", "frequent", "fresh", "friend",
    "fringe", "frog", "front", "frost", "frown", "frozen", "fruit", "fuel",
    "fun", "funny", "furnace", "fury", "future", "gadget", "gain", "galaxy",
    "gallery", "game", "gap", "garage", "garbage", "garden", "garlic", "garment",
    "gas", "gasp", "gate", "gather", "gauge", "gaze", "general", "genius",
    "genre", "gentle", "genuine", "gesture", "ghost", "giant", "gift", "giggle",
    "ginger", "giraffe", "girl", "give", "glad", "glance", "glare", "glass",
    "glide", "glimpse", "globe", "gloom", "glory", "glove", "glow", "glue",
    "goat", "goddess", "gold", "good", "goose", "gorilla", "gospel", "gossip",
    "govern", "gown", "grab", "grace", "grain", "grant", "grape", "grass",
    "gravity", "great", "green", "grid", "grief", "grit", "grocery", "group",
    "grow", "grunt", "guard", "guess", "guide", "guilt", "guitar", "gun",
    "gym", "habit", "hair", "half", "hammer", "hamster", "hand", "happy",
    "harbor", "hard", "harsh", "harvest", "hat", "have", "hawk", "hazard",
    "head", "health", "heart", "heavy", "hedgehog", "height", "hello", "helmet",
    "help", "hen", "hero", "hidden", "high", "hill", "hint", "hip",
    "hire", "history", "hobby", "hockey", "hold", "hole", "holiday", "hollow",
    "home", "honey", "hood", "hope", "horn", "horror", "horse", "hospital",
    "host", "hotel", "hour", "hover", "hub", "huge", "human", "humble",
    "humor", "hundred", "hungry", "hunt", "hurdle", "hurry", "hurt", "husband",
    "hybrid", "ice", "icon", "idea", "identify", "idle", "ignore", "ill",
    "illegal", "illness", "image", "imitate", "immense", "immune", "impact", "impose",
    "improve", "impulse", "inch", "include", "income", "increase", "index", "indicate",
    "indoor", "industry", "infant", "inflict", "inform", "inhale", "inherit", "initial",
    "inject", "injury", "inmate", "inner", "innocent", "input", "inquiry", "insane",
    "insect", "inside", "inspire", "install", "intact", "interest", "into", "invest",
    "invite", "involve", "iron", "island", "isolate", "issue", "item", "ivory",
    "jacket", "jaguar", "jar", "jazz", "jealous", "jeans", "jelly", "jewel",
    "job", "join", "joke", "journey", "joy", "judge", "juice", "jump",
    "jungle", "junior", "junk", "just", "kangaroo", "keen", "keep", "ketchup",
    "key", "kick", "kid", "kidney", "kind", "kingdom", "kiss", "kit",
    "kitchen", "kite", "kitten", "kiwi", "knee", "knife", "knock", "know",
    "lab", "label", "labor", "ladder", "lady", "lake", "lamp", "language",
    "laptop", "large", "later", "latin", "laugh", "laundry", "lava", "law",
    "lawn", "lawsuit", "layer", "lazy", "leader", "leaf", "learn", "leave",
    "lecture", "left", "leg", "legal", "legend", "leisure", "lemon", "lend",
    "length", "lens", "leopard", "lesson", "letter", "level", "liar", "liberty",
    "library", "license", "life", "lift", "light", "like", "limb", "limit",
    "link", "lion", "liquid", "list", "little", "live", "lizard", "load",
    "loan", "lobster", "local", "lock", "logic", "lonely", "long", "loop",
    "lottery", "loud", "lounge", "love", "loyal", "lucky", "luggage", "lumber",
    "lunar", "lunch", "luxury", "lyrics", "machine", "mad", "magic", "magnet",
    "maid", "mail", "main", "major", "make", "mammal", "man", "manage",
    "mandate", "mango", "mansion", "manual", "maple", "marble", "march", "margin",
    "marine", "market", "marriage", "mask", "mass", "master", "match", "material",
    "math", "matrix", "matter", "maximum", "maze", "meadow", "mean", "measure",
    "meat", "mechanic", "medal", "media", "melody", "melt", "member", "memory",
    "mention", "menu", "mercy", "merge", "merit", "merry", "mesh", "message",
    "metal", "method", "middle", "midnight", "milk", "million", "mimic", "mind",
    "minimum", "minor", "minute", "miracle", "mirror", "misery", "miss", "mistake",
    "mix", "mixed", "mixture", "mobile", "model", "modify", "mom", "moment",
    "monitor", "monkey", "monster", "month", "moon", "moral", "more", "morning",
    "mosquito", "mother", "motion", "motor", "mountain", "mouse", "move", "movie",
    "much", "muffin", "mule", "multiply", "muscle", "museum", "mushroom", "music",
    "must", "mutual", "myself", "mystery", "myth", "naive", "name", "napkin",
    "narrow", "nasty", "nation", "nature", "near", "neck", "need", "negative",
    "neglect", "neither", "nephew", "nerve", "nest", "net", "network", "neutral",
    "never", "news", "next", "nice", "night", "noble", "noise", "nominee",
    "noodle", "normal", "north", "nose", "notable", "note", "nothing", "notice",
    "novel", "now", "nuclear", "number", "nurse", "nut", "oak", "obey",
    "object", "oblige", "obscure", "observe", "obtain", "obvious", "occur", "ocean",
    "october", "odor", "off", "offer", "office", "often", "oil", "okay",
    "old", "olive", "olympic", "omit", "once", "one", "onion", "online",
    "only", "open", "opera", "opinion", "oppose", "option", "orange", "orbit",
    "orchard", "order", "ordinary", "organ", "orient", "original", "orphan", "ostrich",
    "other", "outdoor", "outer", "output", "outside", "oval", "oven", "over",
    "own", "owner", "oxygen", "oyster", "ozone", "pact", "paddle", "page",
    "pair", "palace", "palm", "panda", "panel", "panic", "panther", "paper",
    "parade", "parent", "park", "parrot", "party", "pass", "patch", "path",
    "patient", "patrol", "pattern", "pause", "pave", "payment", "peace", "peanut",
    "pear", "peasant", "pelican", "pen", "penalty", "pencil", "people", "pepper",
    "perfect", "permit", "person", "pet", "phone", "photo", "phrase", "physical",
    "piano", "picnic", "picture", "piece", "pig", "pigeon", "pill", "pilot",
    "pink", "pioneer", "pipe", "pistol", "pitch", "pizza", "place", "planet",
    "plastic", "plate", "play", "please", "pledge", "pluck", "plug", "plunge",
    "poem", "poet", "point", "polar", "pole", "police", "pond", "pony",
    "pool", "popular", "portion", "position", "possible", "post", "potato", "pottery",
    "poverty", "powder", "power", "practice", "praise", "predict", "prefer", "prepare",
    "present", "pretty", "prevent", "price", "pride", "primary", "print", "priority",
    "prison", "private", "prize", "problem", "process", "produce", "profit", "program",
    "project", "promote", "proof", "property", "prosper", "protect", "proud", "provide",
    "public", "pudding", "pull", "pulp", "pulse", "pumpkin", "punch", "pupil",
    "puppy", "purchase", "purity", "purpose", "purse", "push", "put", "puzzle",
    "pyramid", "quality", "quantum", "quarter", "question", "quick", "quit", "quiz",
    "quote", "rabbit", "raccoon", "race", "rack", "radar", "radio", "rail",
    "rain", "raise", "rally", "ramp", "ranch", "random", "range", "rapid",
    "rare", "rate", "rather", "raven", "raw", "razor", "ready", "real",
    "reason", "rebel", "rebuild", "recall", "receive", "recipe", "record", "recycle",
    "reduce", "reflect", "reform", "refuse", "region", "regret", "regular", "reject",
    "relax", "release", "relief", "rely", "remain", "remember", "remind", "remove",
    "render", "renew", "rent", "reopen", "repair", "repeat", "replace", "report",
    "require", "rescue", "resemble", "resist", "resource", "response", "result", "retire",
    "retreat", "return", "reunion", "reveal", "review", "reward", "rhythm", "rib",
    "ribbon", "rice", "rich", "ride", "ridge", "rifle", "right", "rigid",
    "ring", "riot", "ripple", "risk", "ritual", "rival", "river", "road",
    "roast", "robot", "robust", "rocket", "romance", "roof", "rookie", "room",
    "rose", "rotate", "rough", "round", "route", "royal", "rubber", "rude",
    "rug", "rule", "run", "runway", "rural", "sad", "saddle", "sadness",
    "safe", "sail", "salad", "salmon", "salon", "salt", "salute", "same",
    "sample", "sand", "satisfy", "satoshi", "sauce", "sausage", "save", "say",
    "scale", "scan", "scare", "scatter", "scene", "scheme", "school", "science",
    "scissors", "scorpion", "scout", "scrap", "screen", "script", "scrub", "sea",
    "search", "season", "seat", "second", "secret", "section", "security", "seed",
    "seek", "segment", "select", "sell", "seminar", "senior", "sense", "sentence",
    "series", "service", "session", "settle", "setup", "seven", "shadow", "shaft",
    "shallow", "share", "shed", "shell", "sheriff", "shield", "shift", "shine",
    "ship", "shiver", "shock", "shoe", "shoot", "shop", "short", "shoulder",
    "shove", "shrimp", "shrug", "shuffle", "shy", "sibling", "sick", "side",
    "siege", "sight", "sign", "silent", "silk", "silly", "silver", "similar",
    "simple", "since", "sing", "siren", "sister", "situate", "six", "size",
    "skate", "sketch", "ski", "skill", "skin", "skirt", "skull", "slab",
    "slam", "sleep", "slender", "slice", "slide", "slight", "slim", "slogan",
    "slot", "slow", "slush", "small", "smart", "smile", "smoke", "smooth",
    "snack", "snake", "snap", "sniff", "snow", "soap", "soccer", "social",
    "sock", "soda", "soft", "solar", "soldier", "solid", "solution", "solve",
    "someone", "song", "soon", "sorry", "sort", "soul", "sound", "soup",
    "source", "south", "space", "spare", "spatial", "spawn", "speak", "special",
    "speed", "spell", "spend", "sphere", "spice", "spider", "spike", "spin",
    "spirit", "split", "spoil", "sponsor", "spoon", "sport", "spot", "spray",
    "spread", "spring", "spy", "square", "squeeze", "squirrel", "stable", "stadium",
    "staff", "stage", "stairs", "stamp", "stand", "start", "state", "stay",
    "steak", "steel", "stem", "step", "stereo", "stick", "still", "sting",
    "stock", "stomach", "stone", "stool", "story", "stove", "strategy", "street",
    "strike", "strong", "struggle", "student", "stuff", "stumble", "style", "subject",
    "submit", "subway", "success", "such", "sudden", "suffer", "sugar", "suggest",
    "suit", "summer", "sun", "sunny", "sunset", "super", "supply", "supreme",
    "sure", "surface", "surge", "surprise", "surround", "survey", "suspect", "sustain",
    "swallow", "swamp", "swap", "swarm", "swear", "sweet", "swift", "swim",
    "swing", "switch", "sword", "symbol", "symptom", "syrup", "system", "table",
    "tackle", "tag", "tail", "talent", "talk", "tank", "tape", "target",
    "task", "taste", "tattoo", "taxi", "teach", "team", "tell", "ten",
    "tenant", "tennis", "tent", "term", "test", "text", "thank", "that",
    "theme", "then", "theory", "there", "they", "thing", "this", "thought",
    "three", "thrive", "throw", "thumb", "thunder", "ticket", "tide", "tiger",
    "tilt", "timber", "time", "tiny", "tip", "tired", "tissue", "title",
    "toast", "tobacco", "today", "toddler", "toe", "together", "toilet", "token",
    "tomato", "tomorrow", "tone", "tongue", "tonight", "tool", "tooth", "top",
    "topic", "topple", "torch", "tornado", "tortoise", "toss", "total", "tourist",
    "toward", "tower", "town", "toy", "track", "trade", "traffic", "tragic",
    "train", "transfer", "trap", "trash", "travel", "tray", "treat", "tree",
    "trend", "trial", "tribe", "trick", "trigger", "trim", "trip", "trophy",
    "trouble", "truck", "true", "truly", "trumpet", "trust", "truth", "try",
    "tube", "tuition", "tumble", "tuna", "tunnel", "turkey", "turn", "turtle",
    "twelve", "twenty", "twice", "twin", "twist", "two", "type", "typical",
    "ugly", "umbrella", "unable", "unaware", "uncle", "uncover", "under", "undo",
    "unfair", "unfold", "unhappy", "uniform", "unique", "unit", "universe", "unknown",
    "unlock", "until", "unusual", "unveil", "update", "upgrade", "uphold", "upon",
    "upper", "upset", "urban", "urge", "usage", "use", "used", "useful",
    "useless", "usual", "utility", "vacant", "vacuum", "vague", "valid", "valley",
    "valve", "van", "vanish", "vapor", "various", "vast", "vault", "vehicle",
    "velvet", "vendor", "venture", "venue", "verb", "verify", "version", "very",
    "vessel", "veteran", "viable", "vibrant", "vicious", "victory", "video", "view",
    "village", "vintage", "violin", "virtual", "virus", "visa", "visit", "visual",
    "vital", "vivid", "vocal", "voice", "void", "volcano", "volume", "vote",
    "voyage", "wage", "wagon", "wait", "walk", "wall", "walnut", "want",
    "warfare", "warm", "warrior", "wash", "wasp", "waste", "water", "wave",
    "way", "wealth", "weapon", "wear", "weasel", "weather", "web", "wedding",
    "weekend", "weird", "welcome", "west", "wet", "whale", "what", "wheat",
    "wheel", "when", "where", "whip", "whisper", "wide", "width", "wife",
    "wild", "will", "win", "window", "wine", "wing", "wink", "winner",
    "winter", "wire", "wisdom", "wise", "wish", "witness", "wolf", "woman",
    "wonder", "wood", "wool", "word", "work", "world", "worry", "worth",
    "wrap", "wreck", "wrestle", "wrist", "write", "wrong", "yard", "year",
    "yellow", "you", "young", "youth", "zebra", "zero", "zone", "zoo"
};

// ============================================================================
// Helper Functions
// ============================================================================

// Extract address from script (used for UTXO scanning)
static std::string ExtractAddressFromScript(const Script& script) {
    // Try P2PKH (most common)
    if (script.IsP2PKH()) {
        auto hash_opt = script.GetP2PKHHash();
        if (hash_opt.has_value()) {
            // Encode pubkey hash as Bech32 address
            auto result = AddressEncoder::EncodeAddress(hash_opt.value());
            if (result.IsOk()) {
                return result.value.value();
            }
        }
    }

    // Try P2PK
    if (script.IsP2PK()) {
        auto pubkey_opt = script.GetP2PKPublicKey();
        if (pubkey_opt.has_value()) {
            // Hash the public key and encode as Bech32 address
            const PublicKey& pubkey = pubkey_opt.value();
            std::vector<uint8_t> pubkey_vec(pubkey.begin(), pubkey.end());
            uint256 pubkey_hash = SHA3::Hash(pubkey_vec);
            auto result = AddressEncoder::EncodeAddress(pubkey_hash);
            if (result.IsOk()) {
                return result.value.value();
            }
        }
    }

    // Unknown script type - return empty string
    return "";
}

// HMAC-SHA256 for BIP32 key derivation (using SHA3-256)
static std::vector<uint8_t> HMAC_SHA256(const std::vector<uint8_t>& key,
                                        const std::vector<uint8_t>& data) {
    // Simplified HMAC-SHA256 using SHA3-256 (our crypto primitive)
    // Note: This is a simplified implementation for wallet key derivation

    const size_t block_size = 64;  // SHA256 block size
    std::vector<uint8_t> k = key;

    // Pad or hash key if needed
    if (k.size() > block_size) {
        auto hash = SHA3::Hash(k);
        k = std::vector<uint8_t>(hash.begin(), hash.end());
    }
    if (k.size() < block_size) {
        k.resize(block_size, 0);
    }

    // Compute inner and outer padding
    std::vector<uint8_t> ipad(block_size), opad(block_size);
    for (size_t i = 0; i < block_size; i++) {
        ipad[i] = k[i] ^ 0x36;
        opad[i] = k[i] ^ 0x5c;
    }

    // Inner hash: H(K XOR ipad || message)
    std::vector<uint8_t> inner_data = ipad;
    inner_data.insert(inner_data.end(), data.begin(), data.end());
    auto inner_hash = SHA3::Hash(inner_data);
    std::vector<uint8_t> inner_bytes(inner_hash.begin(), inner_hash.end());

    // Outer hash: H(K XOR opad || inner_hash)
    std::vector<uint8_t> outer_data = opad;
    outer_data.insert(outer_data.end(), inner_bytes.begin(), inner_bytes.end());
    auto outer_hash = SHA3::Hash(outer_data);
    return std::vector<uint8_t>(outer_hash.begin(), outer_hash.end());
}

// PBKDF2 for BIP39 seed derivation (using HMAC-SHA256)
static std::vector<uint8_t> PBKDF2_HMAC_SHA256(const std::string& password,
                                               const std::string& salt,
                                               size_t iterations,
                                               size_t dkLen) {
    // Simplified PBKDF2 with SHA3-256
    std::vector<uint8_t> pass(password.begin(), password.end());
    std::vector<uint8_t> salt_bytes(salt.begin(), salt.end());

    std::vector<uint8_t> result;
    uint32_t block_count = (dkLen + 31) / 32;  // 32 bytes per SHA256 output

    for (uint32_t i = 1; i <= block_count; i++) {
        // Append block number to salt
        std::vector<uint8_t> salt_block = salt_bytes;
        salt_block.push_back((i >> 24) & 0xFF);
        salt_block.push_back((i >> 16) & 0xFF);
        salt_block.push_back((i >> 8) & 0xFF);
        salt_block.push_back(i & 0xFF);

        // First iteration
        std::vector<uint8_t> u = HMAC_SHA256(pass, salt_block);
        std::vector<uint8_t> block = u;

        // Remaining iterations
        for (size_t j = 1; j < iterations; j++) {
            u = HMAC_SHA256(pass, u);
            for (size_t k = 0; k < u.size() && k < block.size(); k++) {
                block[k] ^= u[k];
            }
        }

        result.insert(result.end(), block.begin(), block.end());
    }

    result.resize(dkLen);
    return result;
}

// Convert bytes to mnemonic indices
static std::vector<size_t> BytesToIndices(const std::vector<uint8_t>& bytes, size_t word_count) {
    std::vector<size_t> indices;

    // Append checksum
    std::vector<uint8_t> data_with_checksum = bytes;
    auto hash = SHA3::Hash(bytes);
    size_t checksum_bits = bytes.size() * 8 / 32;

    // Add checksum bits
    for (size_t i = 0; i < checksum_bits; i++) {
        if (hash[i / 8] & (0x80 >> (i % 8))) {
            data_with_checksum.push_back(0xFF);
        }
    }

    // Convert to 11-bit indices
    size_t bit_pos = 0;
    for (size_t i = 0; i < word_count; i++) {
        size_t index = 0;
        for (size_t j = 0; j < 11; j++) {
            size_t byte_pos = bit_pos / 8;
            size_t bit_offset = bit_pos % 8;

            if (byte_pos < data_with_checksum.size()) {
                if (data_with_checksum[byte_pos] & (0x80 >> bit_offset)) {
                    index |= (1 << (10 - j));
                }
            }
            bit_pos++;
        }
        indices.push_back(index % 2048);
    }

    return indices;
}

// ============================================================================
// BIP32 Derivation Path Implementation
// ============================================================================

DerivationPath::DerivationPath(const std::vector<PathComponent>& components)
    : components_(components) {}

Result<DerivationPath> DerivationPath::Parse(const std::string& path_str) {
    if (path_str.empty() || path_str[0] != 'm') {
        return Result<DerivationPath>::Error("Path must start with 'm'");
    }

    std::vector<PathComponent> components;
    size_t pos = 1;

    while (pos < path_str.length()) {
        if (path_str[pos] != '/') {
            return Result<DerivationPath>::Error("Invalid path format");
        }
        pos++;

        // Parse number
        size_t end = pos;
        while (end < path_str.length() && std::isdigit(path_str[end])) {
            end++;
        }

        if (end == pos) {
            return Result<DerivationPath>::Error("Invalid path component");
        }

        uint32_t index = std::stoul(path_str.substr(pos, end - pos));
        bool hardened = false;

        if (end < path_str.length() && path_str[end] == '\'') {
            hardened = true;
            end++;
        }

        components.push_back(PathComponent(index, hardened));
        pos = end;
    }

    return Result<DerivationPath>::Ok(DerivationPath(components));
}

std::string DerivationPath::ToString() const {
    std::ostringstream oss;
    oss << "m";
    for (const auto& comp : components_) {
        oss << "/" << comp.index;
        if (comp.hardened) {
            oss << "'";
        }
    }
    return oss.str();
}

DerivationPath DerivationPath::Append(uint32_t index, bool hardened) const {
    std::vector<PathComponent> new_components = components_;
    new_components.push_back(PathComponent(index, hardened));
    return DerivationPath(new_components);
}

// ============================================================================
// BIP32 HD Key Derivation Implementation
// ============================================================================

Result<ExtendedKey> HDKeyDerivation::GenerateMaster(const std::vector<uint8_t>& seed) {
    if (seed.size() < 16 || seed.size() > 64) {
        return Result<ExtendedKey>::Error("Seed must be between 16 and 64 bytes");
    }

    // HMAC-SHA256 with key "Bitcoin seed" (BIP32 standard, modified for SHA3-256)
    std::string hmac_key = "Bitcoin seed";
    std::vector<uint8_t> key_bytes(hmac_key.begin(), hmac_key.end());
    auto hmac = HMAC_SHA256(key_bytes, seed);

    ExtendedKey master;
    master.depth = 0;
    master.parent_fingerprint = 0;
    master.child_index = 0;

    // For quantum-resistant crypto (Dilithium3), we cannot derive public keys from private keys
    // like in traditional ECDSA. We must generate complete keypairs deterministically.
    //
    // Solution: Use HMAC output as seed for deterministic key generation
    // DilithiumCrypto::GenerateDeterministicKeyPair() uses a custom RNG seeded with the HMAC
    // This ensures the same seed always produces the same master key
    //
    // Note: BIP32 assumes ECDSA where pubkey = privkey * G (elliptic curve math)
    // Dilithium3 doesn't have this property - keys must be generated as complete pairs

    auto keypair_result = DilithiumCrypto::GenerateDeterministicKeyPair(hmac);
    if (!keypair_result.IsOk()) {
        return Result<ExtendedKey>::Error("Failed to generate master keypair: " + keypair_result.error);
    }

    auto keypair = keypair_result.value.value();
    master.private_key = keypair.secret_key;
    master.public_key = keypair.public_key;

    // Chain code from HMAC for deriving children
    std::copy_n(hmac.begin(), 32, master.chain_code.begin());

    return Result<ExtendedKey>::Ok(master);
}

Result<ExtendedKey> HDKeyDerivation::DeriveChild(const ExtendedKey& parent,
                                                 uint32_t index, bool hardened) {
    ExtendedKey child;
    child.depth = parent.depth + 1;
    child.child_index = hardened ? (index | 0x80000000) : index;

    // Calculate parent fingerprint (first 4 bytes of hash of parent pubkey)
    if (parent.public_key.has_value()) {
        auto pk_bytes = std::vector<uint8_t>(parent.public_key.value().begin(),
                                            parent.public_key.value().end());
        auto hash = SHA3::Hash(pk_bytes);
        child.parent_fingerprint = (hash[0] << 24) | (hash[1] << 16) |
                                  (hash[2] << 8) | hash[3];
    }

    // Prepare data for HMAC
    std::vector<uint8_t> data;

    if (hardened) {
        // Hardened derivation: use private key
        if (!parent.private_key.has_value()) {
            return Result<ExtendedKey>::Error("Cannot derive hardened child from public key");
        }
        data.push_back(0x00);
        data.insert(data.end(), parent.private_key.value().begin(),
                   parent.private_key.value().end());
    } else {
        // Normal derivation: use public key
        if (!parent.public_key.has_value()) {
            return Result<ExtendedKey>::Error("Parent key must have public key");
        }
        data.insert(data.end(), parent.public_key.value().begin(),
                   parent.public_key.value().end());
    }

    // Append index (big-endian)
    uint32_t child_index = hardened ? (index | 0x80000000) : index;
    data.push_back((child_index >> 24) & 0xFF);
    data.push_back((child_index >> 16) & 0xFF);
    data.push_back((child_index >> 8) & 0xFF);
    data.push_back(child_index & 0xFF);

    // Derive child key
    std::vector<uint8_t> chain_vec(parent.chain_code.begin(), parent.chain_code.end());
    auto hmac = HMAC_SHA256(chain_vec, data);

    // Child private key (if parent has private key)
    if (parent.private_key.has_value()) {
        // For quantum-resistant keys (Dilithium3), we cannot derive public from private
        // We must generate a complete keypair deterministically from the HMAC output
        //
        // The HMAC output serves as the derivation material - same parent + index = same child
        // This ensures deterministic key derivation for HD wallets with post-quantum crypto

        auto keypair_result = DilithiumCrypto::GenerateDeterministicKeyPair(hmac);
        if (!keypair_result.IsOk()) {
            return Result<ExtendedKey>::Error("Failed to generate child keypair: " + keypair_result.error);
        }

        auto keypair = keypair_result.value.value();
        child.private_key = keypair.secret_key;
        child.public_key = keypair.public_key;
    }

    // Chain code
    std::copy_n(hmac.begin(), 32, child.chain_code.begin());

    return Result<ExtendedKey>::Ok(child);
}

Result<ExtendedKey> HDKeyDerivation::DerivePath(const ExtendedKey& master,
                                                const DerivationPath& path) {
    ExtendedKey current = master;

    for (const auto& component : path.GetComponents()) {
        auto result = DeriveChild(current, component.index, component.hardened);
        if (!result.IsOk()) {
            return result;
        }
        current = result.value.value();
    }

    return Result<ExtendedKey>::Ok(current);
}

Result<ExtendedKey> HDKeyDerivation::Neuter(const ExtendedKey& private_key) {
    if (!private_key.private_key.has_value()) {
        return Result<ExtendedKey>::Error("Key is already neutered");
    }

    ExtendedKey public_key = private_key;
    public_key.private_key = std::nullopt;

    return Result<ExtendedKey>::Ok(public_key);
}

std::string ExtendedKey::SerializeBase58() const {
    // TODO: Implement Base58Check serialization
    // Note: Implementation ready but requires SHA3_256 function to be linked
    // See util.cpp for Base58 encode/decode functions
    return "xprv_placeholder";
}

Result<ExtendedKey> ExtendedKey::DeserializeBase58(const std::string& str) {
    // TODO: Implement Base58Check deserialization
    // Note: Implementation ready but requires SHA3_256 function to be linked
    // See util.cpp for Base58 encode/decode functions
    (void)str;
    return Result<ExtendedKey>::Error("Not implemented - requires SHA3_256 linkage");
}

// ============================================================================
// BIP39 Mnemonic Implementation
// ============================================================================

Result<std::vector<std::string>> Mnemonic::Generate(size_t word_count) {
    // Validate word count (12, 15, 18, 21, or 24 words)
    if (word_count != 12 && word_count != 15 && word_count != 18 &&
        word_count != 21 && word_count != 24) {
        return Result<std::vector<std::string>>::Error(
            "Word count must be 12, 15, 18, 21, or 24");
    }

    // Calculate entropy size
    size_t entropy_bits = (word_count * 11) - (word_count * 11 / 33);
    size_t entropy_bytes = entropy_bits / 8;

    // Generate random entropy
    auto entropy = RandomGenerator::GetRandomBytes(entropy_bytes);

    // Convert to mnemonic
    auto indices = BytesToIndices(entropy, word_count);
    std::vector<std::string> words;
    for (size_t idx : indices) {
        words.push_back(BIP39_WORDLIST[idx]);
    }

    return Result<std::vector<std::string>>::Ok(words);
}

Result<std::vector<uint8_t>> Mnemonic::ToSeed(const std::vector<std::string>& words,
                                              const std::string& passphrase) {
    // Validate mnemonic first
    auto validate_result = Validate(words);
    if (!validate_result.IsOk()) {
        return Result<std::vector<uint8_t>>::Error(validate_result.error);
    }

    // Create mnemonic string
    std::ostringstream oss;
    for (size_t i = 0; i < words.size(); i++) {
        if (i > 0) oss << " ";
        oss << words[i];
    }
    std::string mnemonic = oss.str();

    // BIP39: salt = "mnemonic" + passphrase
    std::string salt = "mnemonic" + passphrase;

    // Derive seed using PBKDF2-HMAC-SHA256 (modified for SHA3-256)
    auto seed = PBKDF2_HMAC_SHA256(mnemonic, salt, 2048, 64);

    return Result<std::vector<uint8_t>>::Ok(seed);
}

Result<void> Mnemonic::Validate(const std::vector<std::string>& words) {
    // Check word count
    if (words.size() != 12 && words.size() != 15 && words.size() != 18 &&
        words.size() != 21 && words.size() != 24) {
        return Result<void>::Error("Invalid word count");
    }

    // Check each word is in wordlist
    for (const auto& word : words) {
        bool found = false;
        for (const auto& valid_word : BIP39_WORDLIST) {
            if (word == valid_word) {
                found = true;
                break;
            }
        }
        if (!found) {
            return Result<void>::Error("Word not in wordlist: " + word);
        }
    }

    // TODO: Validate checksum

    return Result<void>::Ok();
}

const std::vector<std::string>& Mnemonic::GetWordList() {
    return BIP39_WORDLIST;
}

// ============================================================================
// Wallet Address Implementation
// ============================================================================

uint32_t WalletAddress::GetIndex() const {
    if (path.GetComponents().empty()) {
        return 0;
    }
    return path.GetComponents().back().index;
}

// ============================================================================
// Wallet Database Implementation
// ============================================================================

class WalletDB::Impl {
public:
    std::string wallet_path;
    std::unique_ptr<rocksdb::DB> db;
    bool is_open = false;

    explicit Impl(const std::string& path) : wallet_path(path) {}

    // Helper: Serialize WalletAddress
    std::vector<uint8_t> SerializeAddress(const WalletAddress& addr) {
        std::vector<uint8_t> data;

        // Address string (length-prefixed)
        uint32_t addr_len = addr.address.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&addr_len),
                   reinterpret_cast<uint8_t*>(&addr_len) + sizeof(addr_len));
        data.insert(data.end(), addr.address.begin(), addr.address.end());

        // Public key (fixed size)
        data.insert(data.end(), addr.public_key.begin(), addr.public_key.end());

        // Derivation path (simplified - store as string)
        std::string path_str = addr.path.ToString();
        uint32_t path_len = path_str.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&path_len),
                   reinterpret_cast<uint8_t*>(&path_len) + sizeof(path_len));
        data.insert(data.end(), path_str.begin(), path_str.end());

        // Label (length-prefixed)
        uint32_t label_len = addr.label.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&label_len),
                   reinterpret_cast<uint8_t*>(&label_len) + sizeof(label_len));
        data.insert(data.end(), addr.label.begin(), addr.label.end());

        // Timestamps
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&addr.creation_time),
                   reinterpret_cast<const uint8_t*>(&addr.creation_time) + sizeof(addr.creation_time));
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&addr.last_used_time),
                   reinterpret_cast<const uint8_t*>(&addr.last_used_time) + sizeof(addr.last_used_time));

        // is_change flag
        uint8_t is_change = addr.is_change ? 1 : 0;
        data.push_back(is_change);

        return data;
    }

    // Helper: Deserialize WalletAddress
    Result<WalletAddress> DeserializeAddress(const std::vector<uint8_t>& data) {
        if (data.size() < sizeof(uint32_t)) {
            return Result<WalletAddress>::Error("Invalid address data");
        }

        WalletAddress addr;
        size_t offset = 0;

        // Address string
        uint32_t addr_len;
        std::memcpy(&addr_len, data.data() + offset, sizeof(addr_len));
        offset += sizeof(addr_len);
        if (offset + addr_len > data.size()) {
            return Result<WalletAddress>::Error("Invalid address data (address)");
        }
        addr.address = std::string(data.begin() + offset, data.begin() + offset + addr_len);
        offset += addr_len;

        // Public key
        if (offset + DILITHIUM3_PUBLICKEYBYTES > data.size()) {
            return Result<WalletAddress>::Error("Invalid address data (pubkey)");
        }
        std::copy_n(data.begin() + offset, DILITHIUM3_PUBLICKEYBYTES, addr.public_key.begin());
        offset += DILITHIUM3_PUBLICKEYBYTES;

        // Derivation path
        uint32_t path_len;
        std::memcpy(&path_len, data.data() + offset, sizeof(path_len));
        offset += sizeof(path_len);
        if (offset + path_len > data.size()) {
            return Result<WalletAddress>::Error("Invalid address data (path)");
        }
        std::string path_str(data.begin() + offset, data.begin() + offset + path_len);
        auto path_result = DerivationPath::Parse(path_str);
        if (!path_result.IsOk()) {
            return Result<WalletAddress>::Error("Invalid derivation path");
        }
        addr.path = path_result.value.value();
        offset += path_len;

        // Label
        uint32_t label_len;
        std::memcpy(&label_len, data.data() + offset, sizeof(label_len));
        offset += sizeof(label_len);
        if (offset + label_len > data.size()) {
            return Result<WalletAddress>::Error("Invalid address data (label)");
        }
        addr.label = std::string(data.begin() + offset, data.begin() + offset + label_len);
        offset += label_len;

        // Timestamps
        if (offset + sizeof(uint64_t) * 2 + 1 > data.size()) {
            return Result<WalletAddress>::Error("Invalid address data (timestamps)");
        }
        std::memcpy(&addr.creation_time, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);
        std::memcpy(&addr.last_used_time, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        // is_change flag
        addr.is_change = (data[offset] != 0);

        return Result<WalletAddress>::Ok(addr);
    }

    // Helper: Serialize WalletTransaction
    std::vector<uint8_t> SerializeWalletTransaction(const WalletTransaction& wtx) {
        std::vector<uint8_t> data;

        // txid (32 bytes)
        data.insert(data.end(), wtx.txid.begin(), wtx.txid.end());

        // Transaction (serialize the full transaction)
        auto tx_data = wtx.tx.Serialize();
        uint32_t tx_len = tx_data.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&tx_len),
                   reinterpret_cast<uint8_t*>(&tx_len) + sizeof(tx_len));
        data.insert(data.end(), tx_data.begin(), tx_data.end());

        // block_height (8 bytes)
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&wtx.block_height),
                   reinterpret_cast<const uint8_t*>(&wtx.block_height) + sizeof(wtx.block_height));

        // block_hash (32 bytes)
        data.insert(data.end(), wtx.block_hash.begin(), wtx.block_hash.end());

        // timestamp (8 bytes)
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&wtx.timestamp),
                   reinterpret_cast<const uint8_t*>(&wtx.timestamp) + sizeof(wtx.timestamp));

        // amount (8 bytes, signed)
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&wtx.amount),
                   reinterpret_cast<const uint8_t*>(&wtx.amount) + sizeof(wtx.amount));

        // fee (8 bytes)
        data.insert(data.end(), reinterpret_cast<const uint8_t*>(&wtx.fee),
                   reinterpret_cast<const uint8_t*>(&wtx.fee) + sizeof(wtx.fee));

        // comment (length-prefixed string)
        uint32_t comment_len = wtx.comment.size();
        data.insert(data.end(), reinterpret_cast<uint8_t*>(&comment_len),
                   reinterpret_cast<uint8_t*>(&comment_len) + sizeof(comment_len));
        data.insert(data.end(), wtx.comment.begin(), wtx.comment.end());

        // is_coinbase (1 byte)
        uint8_t is_coinbase = wtx.is_coinbase ? 1 : 0;
        data.push_back(is_coinbase);

        return data;
    }

    // Helper: Deserialize WalletTransaction
    Result<WalletTransaction> DeserializeWalletTransaction(const std::vector<uint8_t>& data) {
        if (data.size() < 32) {
            return Result<WalletTransaction>::Error("Invalid transaction data");
        }

        WalletTransaction wtx;
        size_t offset = 0;

        // txid (32 bytes)
        std::copy_n(data.begin() + offset, 32, wtx.txid.begin());
        offset += 32;

        // Transaction
        if (offset + sizeof(uint32_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (tx length)");
        }
        uint32_t tx_len;
        std::memcpy(&tx_len, data.data() + offset, sizeof(tx_len));
        offset += sizeof(tx_len);

        if (offset + tx_len > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (tx data)");
        }
        std::vector<uint8_t> tx_data(data.begin() + offset, data.begin() + offset + tx_len);
        auto tx_result = Transaction::Deserialize(tx_data);
        if (!tx_result.IsOk()) {
            return Result<WalletTransaction>::Error("Failed to deserialize transaction");
        }
        wtx.tx = tx_result.value.value();
        offset += tx_len;

        // block_height (8 bytes)
        if (offset + sizeof(uint64_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (block_height)");
        }
        std::memcpy(&wtx.block_height, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        // block_hash (32 bytes)
        if (offset + 32 > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (block_hash)");
        }
        std::copy_n(data.begin() + offset, 32, wtx.block_hash.begin());
        offset += 32;

        // timestamp (8 bytes)
        if (offset + sizeof(uint64_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (timestamp)");
        }
        std::memcpy(&wtx.timestamp, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        // amount (8 bytes, signed)
        if (offset + sizeof(int64_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (amount)");
        }
        std::memcpy(&wtx.amount, data.data() + offset, sizeof(int64_t));
        offset += sizeof(int64_t);

        // fee (8 bytes)
        if (offset + sizeof(uint64_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (fee)");
        }
        std::memcpy(&wtx.fee, data.data() + offset, sizeof(uint64_t));
        offset += sizeof(uint64_t);

        // comment (length-prefixed string)
        if (offset + sizeof(uint32_t) > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (comment length)");
        }
        uint32_t comment_len;
        std::memcpy(&comment_len, data.data() + offset, sizeof(comment_len));
        offset += sizeof(comment_len);

        if (offset + comment_len > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (comment)");
        }
        wtx.comment = std::string(data.begin() + offset, data.begin() + offset + comment_len);
        offset += comment_len;

        // is_coinbase (1 byte)
        if (offset + 1 > data.size()) {
            return Result<WalletTransaction>::Error("Invalid transaction data (is_coinbase)");
        }
        wtx.is_coinbase = (data[offset] != 0);

        // Set txid from transaction if not already set
        if (wtx.txid == uint256()) {
            wtx.txid = wtx.tx.GetHash();
        }

        return Result<WalletTransaction>::Ok(wtx);
    }
};

WalletDB::WalletDB(const std::string& wallet_path)
    : impl_(std::make_unique<Impl>(wallet_path)) {}

WalletDB::~WalletDB() {
    if (IsOpen()) {
        Close();
    }
}

Result<void> WalletDB::Open() {
    if (impl_->is_open) {
        return Result<void>::Error("Wallet database already open");
    }

    // Open RocksDB directly
    rocksdb::Options options;
    options.create_if_missing = true;
    options.error_if_exists = false;

    rocksdb::DB* db_ptr = nullptr;
    rocksdb::Status status = rocksdb::DB::Open(options, impl_->wallet_path, &db_ptr);

    if (!status.ok()) {
        return Result<void>::Error("Failed to open wallet database: " + status.ToString());
    }

    impl_->db.reset(db_ptr);
    impl_->is_open = true;
    return Result<void>::Ok();
}

Result<void> WalletDB::Close() {
    if (!impl_->is_open) {
        return Result<void>::Error("Wallet database not open");
    }

    // Close RocksDB
    impl_->db.reset();
    impl_->is_open = false;
    return Result<void>::Ok();
}

bool WalletDB::IsOpen() const {
    return impl_->is_open;
}

Result<void> WalletDB::WriteAddress(const WalletAddress& addr) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    // Serialize address
    auto data = impl_->SerializeAddress(addr);

    // Store with key prefix "addr_"
    std::string key = "addr_" + addr.address;
    rocksdb::Slice key_slice(key);
    rocksdb::Slice value_slice(reinterpret_cast<const char*>(data.data()), data.size());

    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), key_slice, value_slice);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write address: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<WalletAddress> WalletDB::ReadAddress(const std::string& address) {
    if (!impl_->is_open) {
        return Result<WalletAddress>::Error("Database not open");
    }

    // Read from database
    std::string key = "addr_" + address;
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound()) {
        return Result<WalletAddress>::Error("Address not found");
    }

    if (!status.ok()) {
        return Result<WalletAddress>::Error("Failed to read address: " + status.ToString());
    }

    // Deserialize
    std::vector<uint8_t> data(value.begin(), value.end());
    return impl_->DeserializeAddress(data);
}

Result<std::vector<WalletAddress>> WalletDB::ReadAllAddresses() {
    if (!impl_->is_open) {
        return Result<std::vector<WalletAddress>>::Error("Database not open");
    }

    // Iterate through all addresses with "addr_" prefix
    std::vector<WalletAddress> addresses;
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));

    std::string prefix = "addr_";
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        std::string value_str = it->value().ToString();
        std::vector<uint8_t> data(value_str.begin(), value_str.end());

        auto addr_result = impl_->DeserializeAddress(data);
        if (addr_result.IsOk()) {
            addresses.push_back(addr_result.value.value());
        }
    }

    return Result<std::vector<WalletAddress>>::Ok(addresses);
}

Result<void> WalletDB::DeleteAddress(const std::string& address) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "addr_" + address;
    rocksdb::Status status = impl_->db->Delete(rocksdb::WriteOptions(), key);

    if (!status.ok()) {
        return Result<void>::Error("Failed to delete address: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<void> WalletDB::WriteTransaction(const WalletTransaction& wtx) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    auto data = impl_->SerializeWalletTransaction(wtx);

    // Use txid as hex string for the key
    std::string key = "tx_" + ToHex(wtx.txid);
    rocksdb::Slice key_slice(key);
    rocksdb::Slice value_slice(reinterpret_cast<const char*>(data.data()), data.size());

    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), key_slice, value_slice);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write transaction: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<WalletTransaction> WalletDB::ReadTransaction(const uint256& txid) {
    if (!impl_->is_open) {
        return Result<WalletTransaction>::Error("Database not open");
    }

    std::string key = "tx_" + ToHex(txid);
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound()) {
        return Result<WalletTransaction>::Error("Transaction not found");
    }

    if (!status.ok()) {
        return Result<WalletTransaction>::Error("Failed to read transaction: " + status.ToString());
    }

    std::vector<uint8_t> data(value.begin(), value.end());
    return impl_->DeserializeWalletTransaction(data);
}

Result<std::vector<WalletTransaction>> WalletDB::ReadAllTransactions() {
    if (!impl_->is_open) {
        return Result<std::vector<WalletTransaction>>::Error("Database not open");
    }

    std::vector<WalletTransaction> transactions;
    std::unique_ptr<rocksdb::Iterator> it(impl_->db->NewIterator(rocksdb::ReadOptions()));

    std::string prefix = "tx_";
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        std::string value_str = it->value().ToString();
        std::vector<uint8_t> data(value_str.begin(), value_str.end());

        auto wtx_result = impl_->DeserializeWalletTransaction(data);
        if (wtx_result.IsOk()) {
            transactions.push_back(wtx_result.value.value());
        }
    }

    return Result<std::vector<WalletTransaction>>::Ok(transactions);
}

Result<void> WalletDB::DeleteTransaction(const uint256& txid) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "tx_" + ToHex(txid);
    rocksdb::Status status = impl_->db->Delete(rocksdb::WriteOptions(), key);

    if (!status.ok()) {
        return Result<void>::Error("Failed to delete transaction: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<void> WalletDB::WriteMasterKey(const std::vector<uint8_t>& encrypted_seed) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "master_key";
    rocksdb::Slice value_slice(reinterpret_cast<const char*>(encrypted_seed.data()),
                               encrypted_seed.size());
    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), key, value_slice);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write master key: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<std::vector<uint8_t>> WalletDB::ReadMasterKey() {
    if (!impl_->is_open) {
        return Result<std::vector<uint8_t>>::Error("Database not open");
    }

    std::string key = "master_key";
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound()) {
        return Result<std::vector<uint8_t>>::Error("Master key not found");
    }

    if (!status.ok()) {
        return Result<std::vector<uint8_t>>::Error("Failed to read master key: " + status.ToString());
    }

    std::vector<uint8_t> seed(value.begin(), value.end());
    return Result<std::vector<uint8_t>>::Ok(seed);
}

Result<void> WalletDB::WriteMetadata(const std::string& key, const std::string& value) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string db_key = "meta_" + key;
    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), db_key, value);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write metadata: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<std::string> WalletDB::ReadMetadata(const std::string& key) {
    if (!impl_->is_open) {
        return Result<std::string>::Error("Database not open");
    }

    std::string db_key = "meta_" + key;
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), db_key, &value);

    if (status.IsNotFound()) {
        return Result<std::string>::Error("Metadata not found");
    }

    if (!status.ok()) {
        return Result<std::string>::Error("Failed to read metadata: " + status.ToString());
    }

    return Result<std::string>::Ok(value);
}

Result<void> WalletDB::WriteEncryptedSeed(const std::vector<uint8_t>& salt,
                                          const std::vector<uint8_t>& iv,
                                          const std::vector<uint8_t>& encrypted_seed,
                                          const std::vector<uint8_t>& auth_tag) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    // Serialize all components into a single value
    // Format: [salt_len(4)][salt][iv_len(4)][iv][seed_len(4)][seed][tag_len(4)][tag]
    std::vector<uint8_t> data;
    data.reserve(16 + salt.size() + iv.size() + encrypted_seed.size() + auth_tag.size());

    // Write salt length and data
    uint32_t salt_len = static_cast<uint32_t>(salt.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&salt_len), reinterpret_cast<uint8_t*>(&salt_len) + 4);
    data.insert(data.end(), salt.begin(), salt.end());

    // Write IV length and data
    uint32_t iv_len = static_cast<uint32_t>(iv.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&iv_len), reinterpret_cast<uint8_t*>(&iv_len) + 4);
    data.insert(data.end(), iv.begin(), iv.end());

    // Write encrypted seed length and data
    uint32_t seed_len = static_cast<uint32_t>(encrypted_seed.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&seed_len), reinterpret_cast<uint8_t*>(&seed_len) + 4);
    data.insert(data.end(), encrypted_seed.begin(), encrypted_seed.end());

    // Write auth tag length and data
    uint32_t tag_len = static_cast<uint32_t>(auth_tag.size());
    data.insert(data.end(), reinterpret_cast<uint8_t*>(&tag_len), reinterpret_cast<uint8_t*>(&tag_len) + 4);
    data.insert(data.end(), auth_tag.begin(), auth_tag.end());

    // Write to database
    rocksdb::Slice value(reinterpret_cast<const char*>(data.data()), data.size());
    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), "encrypted_seed", value);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write encrypted seed: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<void> WalletDB::ReadEncryptedSeed(std::vector<uint8_t>& salt,
                                         std::vector<uint8_t>& iv,
                                         std::vector<uint8_t>& encrypted_seed,
                                         std::vector<uint8_t>& auth_tag) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), "encrypted_seed", &value);

    if (status.IsNotFound()) {
        return Result<void>::Error("Encrypted seed not found");
    }

    if (!status.ok()) {
        return Result<void>::Error("Failed to read encrypted seed: " + status.ToString());
    }

    // Deserialize components
    const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
    size_t offset = 0;

    // Read salt
    if (offset + 4 > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (salt length)");
    }
    uint32_t salt_len = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (offset + salt_len > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (salt)");
    }
    salt.assign(data + offset, data + offset + salt_len);
    offset += salt_len;

    // Read IV
    if (offset + 4 > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (IV length)");
    }
    uint32_t iv_len = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (offset + iv_len > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (IV)");
    }
    iv.assign(data + offset, data + offset + iv_len);
    offset += iv_len;

    // Read encrypted seed
    if (offset + 4 > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (seed length)");
    }
    uint32_t seed_len = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (offset + seed_len > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (seed)");
    }
    encrypted_seed.assign(data + offset, data + offset + seed_len);
    offset += seed_len;

    // Read auth tag
    if (offset + 4 > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (tag length)");
    }
    uint32_t tag_len = *reinterpret_cast<const uint32_t*>(data + offset);
    offset += 4;

    if (offset + tag_len > value.size()) {
        return Result<void>::Error("Corrupted encrypted seed data (tag)");
    }
    auth_tag.assign(data + offset, data + offset + tag_len);

    return Result<void>::Ok();
}

Result<void> WalletDB::WriteLabel(const std::string& address, const std::string& label) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "label_" + address;
    rocksdb::Status status = impl_->db->Put(rocksdb::WriteOptions(), key, label);

    if (!status.ok()) {
        return Result<void>::Error("Failed to write label: " + status.ToString());
    }

    return Result<void>::Ok();
}

Result<std::string> WalletDB::ReadLabel(const std::string& address) {
    if (!impl_->is_open) {
        return Result<std::string>::Error("Database not open");
    }

    std::string key = "label_" + address;
    std::string value;
    rocksdb::Status status = impl_->db->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound()) {
        return Result<std::string>::Ok("");  // Return empty string if no label
    }

    if (!status.ok()) {
        return Result<std::string>::Error("Failed to read label: " + status.ToString());
    }

    return Result<std::string>::Ok(value);
}

Result<void> WalletDB::Backup(const std::string& backup_path) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    // Create backup engine
    rocksdb::BackupEngine* backup_engine = nullptr;
    rocksdb::BackupEngineOptions backup_options(backup_path);
    backup_options.share_table_files = true;  // Deduplicate files

    rocksdb::Status status = rocksdb::BackupEngine::Open(
        rocksdb::Env::Default(),
        backup_options,
        &backup_engine
    );

    if (!status.ok()) {
        return Result<void>::Error("Failed to create backup engine: " + status.ToString());
    }

    std::unique_ptr<rocksdb::BackupEngine> backup_ptr(backup_engine);

    // Create new backup
    status = backup_ptr->CreateNewBackup(impl_->db.get());
    if (!status.ok()) {
        return Result<void>::Error("Failed to create backup: " + status.ToString());
    }

    return Result<void>::Ok();
}

// ============================================================================
// Wallet Core Implementation
// ============================================================================

class Wallet::Impl {
public:
    WalletConfig config;
    std::unique_ptr<WalletDB> db;
    bool is_loaded = false;
    bool is_encrypted = false;
    bool is_locked = true;

    // Master key data
    ExtendedKey master_key;
    std::vector<std::string> mnemonic_words;

    // Encryption data (AES-256-GCM)
    std::vector<uint8_t> salt;              // 32 bytes for PBKDF2
    std::vector<uint8_t> iv;                // 12 bytes for GCM
    std::vector<uint8_t> encrypted_seed;    // Encrypted master seed
    std::vector<uint8_t> auth_tag;          // 16 bytes GCM authentication tag

    // Address management
    std::vector<WalletAddress> addresses;
    uint32_t next_receive_index = 0;
    uint32_t next_change_index = 0;

    // Transaction history
    std::vector<WalletTransaction> transactions;

    // UTXO set
    std::map<OutPoint, TxOut> utxos;

    explicit Impl(const WalletConfig& cfg) : config(cfg) {}

    // Derive address at index
    Result<WalletAddress> DeriveAddress(uint32_t account, bool is_change, uint32_t index) {
        // BIP44 path: m/44'/2210'/account'/change/index
        DerivationPath path;
        path = path.Append(44, true);      // Purpose
        path = path.Append(config.coin_type, true);  // Coin type
        path = path.Append(account, true);  // Account
        path = path.Append(is_change ? 1 : 0, false);  // Change
        path = path.Append(index, false);   // Address index

        // Derive key
        auto key_result = HDKeyDerivation::DerivePath(master_key, path);
        if (!key_result.IsOk()) {
            return Result<WalletAddress>::Error("Failed to derive key: " + key_result.error);
        }

        ExtendedKey derived_key = key_result.value.value();
        if (!derived_key.public_key.has_value()) {
            return Result<WalletAddress>::Error("Derived key has no public key");
        }

        // Generate Bech32 address
        std::string address = PublicKeyToAddress(derived_key.public_key.value());

        WalletAddress addr;
        addr.address = address;
        addr.public_key = derived_key.public_key.value();
        addr.path = path;
        addr.is_change = is_change;
        addr.creation_time = std::chrono::system_clock::now().time_since_epoch().count();
        addr.last_used_time = 0;

        return Result<WalletAddress>::Ok(addr);
    }
};

Wallet::Wallet(const WalletConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

Wallet::~Wallet() {
    if (IsLoaded()) {
        Close();
    }
}

Result<void> Wallet::Create(const std::vector<std::string>& mnemonic,
                           const std::string& passphrase) {
    if (impl_->is_loaded) {
        return Result<void>::Error("Wallet already loaded");
    }

    // Validate mnemonic
    auto validate_result = Mnemonic::Validate(mnemonic);
    if (!validate_result.IsOk()) {
        return validate_result;
    }

    // Generate seed from mnemonic
    auto seed_result = Mnemonic::ToSeed(mnemonic, passphrase);
    if (!seed_result.IsOk()) {
        return Result<void>::Error("Failed to generate seed: " + seed_result.error);
    }

    // Generate master key
    auto master_result = HDKeyDerivation::GenerateMaster(seed_result.value.value());
    if (!master_result.IsOk()) {
        return Result<void>::Error("Failed to generate master key: " + master_result.error);
    }

    impl_->master_key = master_result.value.value();
    impl_->mnemonic_words = mnemonic;

    // Create database
    impl_->db = std::make_unique<WalletDB>(impl_->config.data_dir + "/wallet.db");
    auto open_result = impl_->db->Open();
    if (!open_result.IsOk()) {
        return Result<void>::Error("Failed to open database: " + open_result.error);
    }

    // Store master key (unencrypted for now)
    auto seed_vec = seed_result.value.value();
    auto write_result = impl_->db->WriteMasterKey(seed_vec);
    if (!write_result.IsOk()) {
        return Result<void>::Error("Failed to write master key: " + write_result.error);
    }

    // Generate initial keypool
    for (uint32_t i = 0; i < impl_->config.keypool_size; i++) {
        auto addr_result = impl_->DeriveAddress(0, false, i);
        if (!addr_result.IsOk()) {
            return Result<void>::Error("Failed to generate address: " + addr_result.error);
        }
        impl_->addresses.push_back(addr_result.value.value());
    }

    // Update next index to be after the keypool
    impl_->next_receive_index = impl_->config.keypool_size;

    impl_->is_loaded = true;
    impl_->is_locked = false;

    return Result<void>::Ok();
}

Result<void> Wallet::Load() {
    if (impl_->is_loaded) {
        return Result<void>::Error("Wallet already loaded");
    }

    // Open database
    impl_->db = std::make_unique<WalletDB>(impl_->config.data_dir + "/wallet.db");
    auto open_result = impl_->db->Open();
    if (!open_result.IsOk()) {
        return Result<void>::Error("Failed to open database: " + open_result.error);
    }

    // Load master key
    auto seed_result = impl_->db->ReadMasterKey();
    if (!seed_result.IsOk()) {
        return Result<void>::Error("Failed to read master key: " + seed_result.error);
    }

    // Generate master key from seed
    auto master_result = HDKeyDerivation::GenerateMaster(seed_result.value.value());
    if (!master_result.IsOk()) {
        return Result<void>::Error("Failed to generate master key: " + master_result.error);
    }

    impl_->master_key = master_result.value.value();

    // Load addresses
    auto addr_result = impl_->db->ReadAllAddresses();
    if (addr_result.IsOk()) {
        impl_->addresses = addr_result.value.value();
    }

    // Load transactions
    auto tx_result = impl_->db->ReadAllTransactions();
    if (tx_result.IsOk()) {
        impl_->transactions = tx_result.value.value();
    }

    impl_->is_loaded = true;
    impl_->is_locked = impl_->is_encrypted;

    return Result<void>::Ok();
}

Result<void> Wallet::Close() {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    if (impl_->db && impl_->db->IsOpen()) {
        auto close_result = impl_->db->Close();
        if (!close_result.IsOk()) {
            return close_result;
        }
    }

    impl_->is_loaded = false;
    return Result<void>::Ok();
}

bool Wallet::IsLoaded() const {
    return impl_->is_loaded;
}

Result<void> Wallet::Encrypt(const std::string& passphrase) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    if (impl_->is_encrypted) {
        return Result<void>::Error("Wallet already encrypted");
    }

    if (passphrase.empty()) {
        return Result<void>::Error("Passphrase cannot be empty");
    }

    // Serialize master seed (chain code + private key)
    std::vector<uint8_t> seed;
    seed.reserve(64); // 32 bytes chain code + 32 bytes private key

    // Add chain code
    seed.insert(seed.end(), impl_->master_key.chain_code.begin(), impl_->master_key.chain_code.end());

    // Add private key
    if (!impl_->master_key.private_key.has_value()) {
        return Result<void>::Error("No private key to encrypt");
    }
    const auto& sk = impl_->master_key.private_key.value();
    seed.insert(seed.end(), sk.data(), sk.data() + 32);

    // Generate random salt (32 bytes)
    impl_->salt.resize(32);
    if (RAND_bytes(impl_->salt.data(), 32) != 1) {
        return Result<void>::Error("Failed to generate random salt");
    }

    // Derive encryption key using PBKDF2-HMAC-SHA256 (100,000 iterations)
    std::vector<uint8_t> derived_key(32); // 256-bit key
    constexpr int iterations = 100000;

    if (PKCS5_PBKDF2_HMAC(
            passphrase.c_str(), passphrase.length(),
            impl_->salt.data(), impl_->salt.size(),
            iterations,
            EVP_sha256(),
            32, derived_key.data()) != 1) {
        return Result<void>::Error("Failed to derive encryption key");
    }

    // Generate random IV (12 bytes for GCM)
    impl_->iv.resize(12);
    if (RAND_bytes(impl_->iv.data(), 12) != 1) {
        return Result<void>::Error("Failed to generate random IV");
    }

    // Encrypt with AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<void>::Error("Failed to create cipher context");
    }

    int len = 0;
    impl_->encrypted_seed.resize(seed.size());
    impl_->auth_tag.resize(16); // GCM tag is 16 bytes

    do {
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derived_key.data(), impl_->iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Result<void>::Error("Failed to initialize encryption");
        }

        // Encrypt the seed
        if (EVP_EncryptUpdate(ctx, impl_->encrypted_seed.data(), &len, seed.data(), seed.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Result<void>::Error("Failed to encrypt seed");
        }

        // Finalize encryption
        int final_len = 0;
        if (EVP_EncryptFinal_ex(ctx, impl_->encrypted_seed.data() + len, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Result<void>::Error("Failed to finalize encryption");
        }

        // Get authentication tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, impl_->auth_tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return Result<void>::Error("Failed to get authentication tag");
        }

    } while (false);

    EVP_CIPHER_CTX_free(ctx);

    // Clear sensitive data from memory
    OPENSSL_cleanse(seed.data(), seed.size());
    OPENSSL_cleanse(derived_key.data(), derived_key.size());

    // Save encrypted data to database
    auto save_result = impl_->db->WriteEncryptedSeed(impl_->salt, impl_->iv, impl_->encrypted_seed, impl_->auth_tag);
    if (!save_result.IsOk()) {
        return Result<void>::Error("Failed to save encrypted data: " + save_result.error);
    }

    impl_->is_encrypted = true;
    impl_->is_locked = true;

    return Result<void>::Ok();
}

Result<void> Wallet::Unlock(const std::string& passphrase, uint32_t timeout_seconds) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    if (!impl_->is_encrypted) {
        return Result<void>::Error("Wallet not encrypted");
    }

    if (!impl_->is_locked) {
        return Result<void>::Error("Wallet already unlocked");
    }

    if (passphrase.empty()) {
        return Result<void>::Error("Passphrase cannot be empty");
    }

    // Load encrypted data from database if not already in memory
    if (impl_->salt.empty() || impl_->iv.empty() || impl_->encrypted_seed.empty() || impl_->auth_tag.empty()) {
        auto load_result = impl_->db->ReadEncryptedSeed(impl_->salt, impl_->iv, impl_->encrypted_seed, impl_->auth_tag);
        if (!load_result.IsOk()) {
            return Result<void>::Error("Failed to load encrypted data: " + load_result.error);
        }
    }

    // Derive encryption key using PBKDF2-HMAC-SHA256 (same parameters as encryption)
    std::vector<uint8_t> derived_key(32);
    constexpr int iterations = 100000;

    if (PKCS5_PBKDF2_HMAC(
            passphrase.c_str(), passphrase.length(),
            impl_->salt.data(), impl_->salt.size(),
            iterations,
            EVP_sha256(),
            32, derived_key.data()) != 1) {
        return Result<void>::Error("Failed to derive decryption key");
    }

    // Decrypt with AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return Result<void>::Error("Failed to create cipher context");
    }

    std::vector<uint8_t> decrypted_seed(impl_->encrypted_seed.size());
    int len = 0;

    do {
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, derived_key.data(), impl_->iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(derived_key.data(), derived_key.size());
            return Result<void>::Error("Failed to initialize decryption");
        }

        // Decrypt the seed
        if (EVP_DecryptUpdate(ctx, decrypted_seed.data(), &len, impl_->encrypted_seed.data(), impl_->encrypted_seed.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(derived_key.data(), derived_key.size());
            return Result<void>::Error("Failed to decrypt seed");
        }

        // Set expected authentication tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, impl_->auth_tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(derived_key.data(), derived_key.size());
            return Result<void>::Error("Failed to set authentication tag");
        }

        // Finalize decryption (verifies authentication tag)
        int final_len = 0;
        if (EVP_DecryptFinal_ex(ctx, decrypted_seed.data() + len, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(derived_key.data(), derived_key.size());
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            return Result<void>::Error("Incorrect passphrase or corrupted data");
        }

    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(derived_key.data(), derived_key.size());

    // Reconstruct master key from decrypted seed
    if (decrypted_seed.size() != 64) {
        OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
        return Result<void>::Error("Invalid seed size");
    }

    // Extract chain code (first 32 bytes)
    std::copy(decrypted_seed.begin(), decrypted_seed.begin() + 32, impl_->master_key.chain_code.begin());

    // Extract private key (last 32 bytes)
    SecretKey sk;
    std::copy(decrypted_seed.begin() + 32, decrypted_seed.end(), sk.data());
    impl_->master_key.private_key = sk;

    // Clear decrypted seed from memory
    OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());

    impl_->is_locked = false;

    // TODO: Handle timeout_seconds for auto-locking (future enhancement)
    (void)timeout_seconds;

    return Result<void>::Ok();
}

Result<void> Wallet::Lock() {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    if (!impl_->is_encrypted) {
        return Result<void>::Error("Wallet not encrypted");
    }

    impl_->is_locked = true;
    return Result<void>::Ok();
}

bool Wallet::IsEncrypted() const {
    return impl_->is_encrypted;
}

bool Wallet::IsLocked() const {
    return impl_->is_locked;
}

Result<void> Wallet::ChangePassphrase(const std::string& old_pass,
                                     const std::string& new_pass) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    if (!impl_->is_encrypted) {
        return Result<void>::Error("Wallet not encrypted");
    }

    if (old_pass.empty() || new_pass.empty()) {
        return Result<void>::Error("Passphrases cannot be empty");
    }

    if (old_pass == new_pass) {
        return Result<void>::Error("New passphrase must be different from old passphrase");
    }

    // Load encrypted data from database if not already in memory
    if (impl_->salt.empty() || impl_->iv.empty() || impl_->encrypted_seed.empty() || impl_->auth_tag.empty()) {
        auto load_result = impl_->db->ReadEncryptedSeed(impl_->salt, impl_->iv, impl_->encrypted_seed, impl_->auth_tag);
        if (!load_result.IsOk()) {
            return Result<void>::Error("Failed to load encrypted data: " + load_result.error);
        }
    }

    // Derive old encryption key
    std::vector<uint8_t> old_key(32);
    constexpr int iterations = 100000;

    if (PKCS5_PBKDF2_HMAC(
            old_pass.c_str(), old_pass.length(),
            impl_->salt.data(), impl_->salt.size(),
            iterations,
            EVP_sha256(),
            32, old_key.data()) != 1) {
        return Result<void>::Error("Failed to derive old decryption key");
    }

    // Decrypt with old passphrase
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(old_key.data(), old_key.size());
        return Result<void>::Error("Failed to create cipher context");
    }

    std::vector<uint8_t> decrypted_seed(impl_->encrypted_seed.size());
    int len = 0;

    do {
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, old_key.data(), impl_->iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(old_key.data(), old_key.size());
            return Result<void>::Error("Failed to initialize decryption");
        }

        // Decrypt the seed
        if (EVP_DecryptUpdate(ctx, decrypted_seed.data(), &len, impl_->encrypted_seed.data(), impl_->encrypted_seed.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(old_key.data(), old_key.size());
            return Result<void>::Error("Failed to decrypt seed");
        }

        // Set expected authentication tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, impl_->auth_tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(old_key.data(), old_key.size());
            return Result<void>::Error("Failed to set authentication tag");
        }

        // Finalize decryption (verifies authentication tag)
        int final_len = 0;
        if (EVP_DecryptFinal_ex(ctx, decrypted_seed.data() + len, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(old_key.data(), old_key.size());
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            return Result<void>::Error("Incorrect old passphrase");
        }

    } while (false);

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(old_key.data(), old_key.size());

    // Now re-encrypt with new passphrase
    // Generate new salt (32 bytes)
    std::vector<uint8_t> new_salt(32);
    if (RAND_bytes(new_salt.data(), 32) != 1) {
        OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
        return Result<void>::Error("Failed to generate new salt");
    }

    // Derive new encryption key
    std::vector<uint8_t> new_key(32);
    if (PKCS5_PBKDF2_HMAC(
            new_pass.c_str(), new_pass.length(),
            new_salt.data(), new_salt.size(),
            iterations,
            EVP_sha256(),
            32, new_key.data()) != 1) {
        OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
        return Result<void>::Error("Failed to derive new encryption key");
    }

    // Generate new IV (12 bytes for GCM)
    std::vector<uint8_t> new_iv(12);
    if (RAND_bytes(new_iv.data(), 12) != 1) {
        OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
        OPENSSL_cleanse(new_key.data(), new_key.size());
        return Result<void>::Error("Failed to generate new IV");
    }

    // Encrypt with AES-256-GCM
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
        OPENSSL_cleanse(new_key.data(), new_key.size());
        return Result<void>::Error("Failed to create cipher context");
    }

    std::vector<uint8_t> new_encrypted_seed(decrypted_seed.size());
    std::vector<uint8_t> new_auth_tag(16);

    do {
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, new_key.data(), new_iv.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            OPENSSL_cleanse(new_key.data(), new_key.size());
            return Result<void>::Error("Failed to initialize encryption");
        }

        // Encrypt the seed
        if (EVP_EncryptUpdate(ctx, new_encrypted_seed.data(), &len, decrypted_seed.data(), decrypted_seed.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            OPENSSL_cleanse(new_key.data(), new_key.size());
            return Result<void>::Error("Failed to encrypt seed");
        }

        // Finalize encryption
        int final_len = 0;
        if (EVP_EncryptFinal_ex(ctx, new_encrypted_seed.data() + len, &final_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            OPENSSL_cleanse(new_key.data(), new_key.size());
            return Result<void>::Error("Failed to finalize encryption");
        }

        // Get authentication tag
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, new_auth_tag.data()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
            OPENSSL_cleanse(new_key.data(), new_key.size());
            return Result<void>::Error("Failed to get authentication tag");
        }

    } while (false);

    EVP_CIPHER_CTX_free(ctx);

    // Clear sensitive data from memory
    OPENSSL_cleanse(decrypted_seed.data(), decrypted_seed.size());
    OPENSSL_cleanse(new_key.data(), new_key.size());

    // Update stored encryption data
    impl_->salt = new_salt;
    impl_->iv = new_iv;
    impl_->encrypted_seed = new_encrypted_seed;
    impl_->auth_tag = new_auth_tag;

    // Save new encrypted data to database
    auto save_result = impl_->db->WriteEncryptedSeed(impl_->salt, impl_->iv, impl_->encrypted_seed, impl_->auth_tag);
    if (!save_result.IsOk()) {
        return Result<void>::Error("Failed to save new encrypted data: " + save_result.error);
    }

    return Result<void>::Ok();
}

Result<std::string> Wallet::GetNewAddress(const std::string& label) {
    if (!impl_->is_loaded) {
        return Result<std::string>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<std::string>::Error("Wallet is locked");
    }

    // Derive new address
    uint32_t index = impl_->next_receive_index++;
    auto addr_result = impl_->DeriveAddress(0, false, index);
    if (!addr_result.IsOk()) {
        return Result<std::string>::Error("Failed to derive address: " + addr_result.error);
    }

    WalletAddress addr = addr_result.value.value();
    addr.label = label;

    // Save to database
    auto write_result = impl_->db->WriteAddress(addr);
    if (!write_result.IsOk()) {
        return Result<std::string>::Error("Failed to save address: " + write_result.error);
    }

    if (!label.empty()) {
        impl_->db->WriteLabel(addr.address, label);
    }

    impl_->addresses.push_back(addr);

    return Result<std::string>::Ok(addr.address);
}

Result<std::string> Wallet::GetNewChangeAddress() {
    if (!impl_->is_loaded) {
        return Result<std::string>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<std::string>::Error("Wallet is locked");
    }

    // Derive new change address
    uint32_t index = impl_->next_change_index++;
    auto addr_result = impl_->DeriveAddress(0, true, index);
    if (!addr_result.IsOk()) {
        return Result<std::string>::Error("Failed to derive address: " + addr_result.error);
    }

    WalletAddress addr = addr_result.value.value();
    addr.is_change = true;

    // Save to database
    auto write_result = impl_->db->WriteAddress(addr);
    if (!write_result.IsOk()) {
        return Result<std::string>::Error("Failed to save address: " + write_result.error);
    }

    impl_->addresses.push_back(addr);

    return Result<std::string>::Ok(addr.address);
}

Result<std::vector<WalletAddress>> Wallet::GetAddresses() const {
    if (!impl_->is_loaded) {
        return Result<std::vector<WalletAddress>>::Error("Wallet not loaded");
    }

    return Result<std::vector<WalletAddress>>::Ok(impl_->addresses);
}

Result<void> Wallet::SetAddressLabel(const std::string& address, const std::string& label) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    return impl_->db->WriteLabel(address, label);
}

Result<std::string> Wallet::GetAddressLabel(const std::string& address) const {
    if (!impl_->is_loaded) {
        return Result<std::string>::Error("Wallet not loaded");
    }

    return impl_->db->ReadLabel(address);
}

Result<uint64_t> Wallet::GetBalance() const {
    if (!impl_->is_loaded) {
        return Result<uint64_t>::Error("Wallet not loaded");
    }

    uint64_t balance = 0;
    for (const auto& [outpoint, txout] : impl_->utxos) {
        balance += txout.value;
    }

    return Result<uint64_t>::Ok(balance);
}

Result<uint64_t> Wallet::GetUnconfirmedBalance() const {
    if (!impl_->is_loaded) {
        return Result<uint64_t>::Error("Wallet not loaded");
    }

    // Calculate balance from unconfirmed transactions (block_height == 0)
    uint64_t unconfirmed_balance = 0;

    for (const auto& wtx : impl_->transactions) {
        if (wtx.block_height == 0) {
            // Check outputs for our addresses
            for (const auto& output : wtx.tx.outputs) {
                std::string addr = ExtractAddressFromScript(output.script_pubkey);

                // Check if this address belongs to us
                bool is_ours = false;
                for (const auto& wallet_addr : impl_->addresses) {
                    if (wallet_addr.address == addr) {
                        is_ours = true;
                        break;
                    }
                }

                if (is_ours) {
                    unconfirmed_balance += output.value;
                }
            }
        }
    }

    return Result<uint64_t>::Ok(unconfirmed_balance);
}

Result<uint64_t> Wallet::GetAddressBalance(const std::string& address) const {
    if (!impl_->is_loaded) {
        return Result<uint64_t>::Error("Wallet not loaded");
    }

    // Calculate balance for specific address from UTXOs
    uint64_t balance = 0;

    for (const auto& [outpoint, txout] : impl_->utxos) {
        // Extract address from script
        std::string utxo_address = ExtractAddressFromScript(txout.script_pubkey);

        if (utxo_address == address) {
            balance += txout.value;
        }
    }

    return Result<uint64_t>::Ok(balance);
}

Result<std::vector<WalletTransaction>> Wallet::GetTransactions() const {
    if (!impl_->is_loaded) {
        return Result<std::vector<WalletTransaction>>::Error("Wallet not loaded");
    }

    return Result<std::vector<WalletTransaction>>::Ok(impl_->transactions);
}

Result<WalletTransaction> Wallet::GetTransaction(const uint256& txid) const {
    if (!impl_->is_loaded) {
        return Result<WalletTransaction>::Error("Wallet not loaded");
    }

    return impl_->db->ReadTransaction(txid);
}

Result<Transaction> Wallet::CreateTransaction(const std::vector<Recipient>& recipients,
                                              uint64_t fee_rate,
                                              const std::string& comment,
                                              CoinSelectionStrategy strategy) {
    if (!impl_->is_loaded) {
        return Result<Transaction>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<Transaction>::Error("Wallet is locked");
    }

    // Validate recipients
    if (recipients.empty()) {
        return Result<Transaction>::Error("No recipients specified");
    }

    // Calculate total output amount
    uint64_t total_output = 0;
    for (const auto& recipient : recipients) {
        if (recipient.amount == 0) {
            return Result<Transaction>::Error("Recipient amount cannot be zero");
        }
        total_output += recipient.amount;
    }

    // Estimate fee (simplified: fee_rate per KB * estimated size)
    // Rough estimate: 200 bytes per input, 100 bytes per output, 100 bytes overhead
    size_t estimated_inputs = std::min(impl_->utxos.size(), size_t(10)); // Estimate max 10 inputs
    size_t estimated_size = 100 + (estimated_inputs * 200) + ((recipients.size() + 1) * 100);
    uint64_t estimated_fee = (fee_rate * estimated_size) / 1000;

    // Select UTXOs using specified strategy
    std::vector<std::pair<OutPoint, TxOut>> selected_utxos;
    uint64_t selected_amount = 0;
    uint64_t target_amount = total_output + estimated_fee;

    // Copy UTXOs to vector for sorting
    std::vector<std::pair<OutPoint, TxOut>> available_utxos;
    for (const auto& [outpoint, txout] : impl_->utxos) {
        available_utxos.push_back({outpoint, txout});
    }

    // Apply coin selection strategy
    switch (strategy) {
        case CoinSelectionStrategy::GREEDY:
            // Select coins in order until target is reached
            for (const auto& [outpoint, txout] : available_utxos) {
                selected_utxos.push_back({outpoint, txout});
                selected_amount += txout.value;
                if (selected_amount >= target_amount) break;
            }
            break;

        case CoinSelectionStrategy::LARGEST_FIRST:
            // Sort by value (descending), select largest coins first
            std::sort(available_utxos.begin(), available_utxos.end(),
                [](const auto& a, const auto& b) { return a.second.value > b.second.value; });
            for (const auto& [outpoint, txout] : available_utxos) {
                selected_utxos.push_back({outpoint, txout});
                selected_amount += txout.value;
                if (selected_amount >= target_amount) break;
            }
            break;

        case CoinSelectionStrategy::SMALLEST_FIRST:
            // Sort by value (ascending), select smallest coins first
            std::sort(available_utxos.begin(), available_utxos.end(),
                [](const auto& a, const auto& b) { return a.second.value < b.second.value; });
            for (const auto& [outpoint, txout] : available_utxos) {
                selected_utxos.push_back({outpoint, txout});
                selected_amount += txout.value;
                if (selected_amount >= target_amount) break;
            }
            break;

        case CoinSelectionStrategy::BRANCH_AND_BOUND: {
            // Branch and bound: optimal UTXO selection algorithm
            // Minimizes transaction fee by finding exact matches or best fit with minimum waste
            //
            // Algorithm overview:
            // 1. Sort UTXOs by descending value for efficient pruning
            // 2. Use depth-first search to explore subset combinations
            // 3. Prune branches that can't improve the current best solution
            // 4. Find exact match (no change) or minimize waste (excess amount)
            //
            // Cost model:
            // - cost_of_change: estimated cost of creating a change output (~68 bytes * fee_rate)
            // - waste = (selected_amount - target_amount) + cost_of_change
            // - Goal: minimize waste or find exact match (waste = 0)

            // Define constants used in algorithm
            const uint64_t DUST_THRESHOLD = 546; // Minimum output value (satoshis)
            const uint64_t CHANGE_OUTPUT_SIZE = 68; // Average change output size (bytes)

            // Sort UTXOs by descending value for better pruning
            std::sort(available_utxos.begin(), available_utxos.end(),
                [](const auto& a, const auto& b) { return a.second.value > b.second.value; });

            // Calculate cost of change output
            uint64_t cost_of_change = (fee_rate * CHANGE_OUTPUT_SIZE) / 1000;

            // Early exit: if total available is less than target, fail immediately
            uint64_t total_available = 0;
            for (const auto& [outpoint, txout] : available_utxos) {
                total_available += txout.value;
            }
            if (total_available < target_amount) {
                // Will be caught by insufficient funds check later
                break;
            }

            // Branch and bound state
            struct BnBState {
                std::vector<std::pair<OutPoint, TxOut>> selected;
                uint64_t selected_value = 0;
                size_t index = 0;
                uint64_t waste = UINT64_MAX;
            };

            BnBState best_solution;
            best_solution.waste = UINT64_MAX;

            // Depth-first search with pruning
            std::function<void(BnBState&, size_t, uint64_t)> bnb_search;
            bnb_search = [&](BnBState& current, size_t depth, uint64_t accumulated_waste) {
                // Pruning: if current waste already exceeds best, stop exploring
                if (accumulated_waste >= best_solution.waste) {
                    return;
                }

                // Base case: explored all UTXOs
                if (depth >= available_utxos.size()) {
                    // Check if this solution is valid (meets target)
                    if (current.selected_value >= target_amount) {
                        uint64_t excess = current.selected_value - target_amount;
                        uint64_t waste = (excess < DUST_THRESHOLD) ? excess : (excess + cost_of_change);

                        if (waste < best_solution.waste) {
                            best_solution = current;
                            best_solution.waste = waste;

                            // Perfect match found (no change needed)
                            if (excess == 0 || excess < DUST_THRESHOLD) {
                                best_solution.waste = 0;
                            }
                        }
                    }
                    return;
                }

                // Get current UTXO
                const auto& [outpoint, txout] = available_utxos[depth];
                uint64_t utxo_value = txout.value;

                // Branch 1: Include this UTXO
                BnBState include_state = current;
                include_state.selected.push_back({outpoint, txout});
                include_state.selected_value += utxo_value;

                // Check if including this UTXO could lead to exact match
                if (include_state.selected_value == target_amount) {
                    // Exact match! Best possible solution (waste = 0)
                    best_solution = include_state;
                    best_solution.waste = 0;
                    return;
                }

                // Pruning: only explore if we haven't exceeded target by too much
                uint64_t max_excess = target_amount + cost_of_change;
                if (include_state.selected_value <= max_excess ||
                    include_state.selected_value < target_amount) {
                    bnb_search(include_state, depth + 1, accumulated_waste);
                }

                // Early exit if we found perfect solution
                if (best_solution.waste == 0) {
                    return;
                }

                // Branch 2: Exclude this UTXO
                // Only explore exclusion if remaining UTXOs could still meet target
                uint64_t remaining_value = 0;
                for (size_t i = depth + 1; i < available_utxos.size(); ++i) {
                    remaining_value += available_utxos[i].second.value;
                }

                if (current.selected_value + remaining_value >= target_amount) {
                    bnb_search(current, depth + 1, accumulated_waste);
                }
            };

            // Start branch and bound search
            BnBState initial_state;
            bnb_search(initial_state, 0, 0);

            // Use best solution found
            if (best_solution.waste != UINT64_MAX && !best_solution.selected.empty()) {
                selected_utxos = best_solution.selected;
                selected_amount = best_solution.selected_value;
            } else {
                // Fallback to largest-first if BnB fails (shouldn't happen with enough UTXOs)
                std::sort(available_utxos.begin(), available_utxos.end(),
                    [](const auto& a, const auto& b) { return a.second.value > b.second.value; });
                for (const auto& [outpoint, txout] : available_utxos) {
                    selected_utxos.push_back({outpoint, txout});
                    selected_amount += txout.value;
                    if (selected_amount >= target_amount) break;
                }
            }
            break;
        }

        case CoinSelectionStrategy::RANDOM: {
            // Random selection for privacy
            auto shuffled = available_utxos;
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(shuffled.begin(), shuffled.end(), g);
            for (const auto& [outpoint, txout] : shuffled) {
                selected_utxos.push_back({outpoint, txout});
                selected_amount += txout.value;
                if (selected_amount >= target_amount) break;
            }
            break;
        }
    }

    // Check if we have enough funds
    if (selected_amount < target_amount) {
        return Result<Transaction>::Error("Insufficient funds");
    }

    // Calculate change
    uint64_t change_amount = selected_amount - total_output - estimated_fee;
    const uint64_t DUST_THRESHOLD = 546; // Minimum output value

    // Create transaction
    Transaction tx;
    tx.version = 1;
    tx.locktime = 0;

    // Create inputs
    for (const auto& [outpoint, txout] : selected_utxos) {
        TxIn input;
        input.prev_tx_hash = outpoint.tx_hash;
        input.prev_tx_index = outpoint.index;
        // script_sig will be filled in SignTransaction()
        input.sequence = 0xFFFFFFFF;
        tx.inputs.push_back(input);
    }

    // Create outputs for recipients
    for (const auto& recipient : recipients) {
        // Decode recipient address to get pubkey hash
        auto decode_result = AddressEncoder::DecodeAddress(recipient.address);
        if (!decode_result.IsOk()) {
            return Result<Transaction>::Error("Invalid recipient address: " + recipient.address);
        }

        TxOut output;
        output.value = recipient.amount;
        output.script_pubkey = Script::CreateP2PKH(decode_result.value.value());
        tx.outputs.push_back(output);
    }

    // Add change output if above dust threshold
    if (change_amount >= DUST_THRESHOLD) {
        // Generate change address
        auto change_addr_result = GetNewChangeAddress();
        if (!change_addr_result.IsOk()) {
            return Result<Transaction>::Error("Failed to generate change address: " + change_addr_result.error);
        }

        std::string change_address = change_addr_result.value.value();
        auto decode_result = AddressEncoder::DecodeAddress(change_address);
        if (!decode_result.IsOk()) {
            return Result<Transaction>::Error("Failed to decode change address");
        }

        TxOut change_output;
        change_output.value = change_amount;
        change_output.script_pubkey = Script::CreateP2PKH(decode_result.value.value());
        tx.outputs.push_back(change_output);
    }

    return Result<Transaction>::Ok(tx);
}

Result<Transaction> Wallet::SignTransaction(const Transaction& tx) {
    if (!impl_->is_loaded) {
        return Result<Transaction>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<Transaction>::Error("Wallet is locked");
    }

    // Create a copy of the transaction to sign
    Transaction signed_tx = tx;

    // Sign each input
    for (size_t i = 0; i < signed_tx.inputs.size(); i++) {
        TxIn& input = signed_tx.inputs[i];

        // Find the UTXO being spent
        OutPoint prevout;
        prevout.tx_hash = input.prev_tx_hash;
        prevout.index = input.prev_tx_index;

        auto utxo_it = impl_->utxos.find(prevout);
        if (utxo_it == impl_->utxos.end()) {
            return Result<Transaction>::Error("UTXO not found for input " + std::to_string(i));
        }

        const TxOut& prev_output = utxo_it->second;

        // Extract address from the previous output script
        std::string address = ExtractAddressFromScript(prev_output.script_pubkey);
        if (address.empty()) {
            return Result<Transaction>::Error("Could not extract address from UTXO script");
        }

        // Find the wallet address
        WalletAddress* wallet_addr = nullptr;
        for (auto& addr : impl_->addresses) {
            if (addr.address == address) {
                wallet_addr = &addr;
                break;
            }
        }

        if (!wallet_addr) {
            return Result<Transaction>::Error("Address not found in wallet: " + address);
        }

        // Derive the key for this address using its path
        auto key_result = HDKeyDerivation::DerivePath(impl_->master_key, wallet_addr->path);
        if (!key_result.IsOk()) {
            return Result<Transaction>::Error("Failed to derive key: " + key_result.error);
        }

        ExtendedKey derived_key = key_result.value.value();
        if (!derived_key.private_key.has_value()) {
            return Result<Transaction>::Error("Derived key has no private key");
        }

        // Get public key for verification
        if (!derived_key.public_key.has_value()) {
            return Result<Transaction>::Error("Derived key has no public key");
        }

        const SecretKey& secret_key = derived_key.private_key.value();
        const PublicKey& public_key = derived_key.public_key.value();

        // Create transaction hash for signing
        // For now, we'll sign the entire transaction (simplified)
        // In production, this should be more sophisticated (e.g., SIGHASH types)
        uint256 tx_hash = signed_tx.GetHash();

        // Sign with Dilithium3
        auto sign_result = DilithiumCrypto::SignHash(tx_hash, secret_key);
        if (!sign_result.IsOk()) {
            return Result<Transaction>::Error("Failed to sign transaction: " + sign_result.error);
        }

        Signature signature = sign_result.value.value();

        // Create script_sig (P2PKH: <signature> <pubkey>)
        // For now, use a simplified format: OP_PUSHDATA + length + data
        std::vector<uint8_t> script_data;

        // Add signature
        script_data.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
        uint32_t sig_len = signature.size();
        script_data.insert(script_data.end(),
                          reinterpret_cast<uint8_t*>(&sig_len),
                          reinterpret_cast<uint8_t*>(&sig_len) + sizeof(sig_len));
        script_data.insert(script_data.end(), signature.begin(), signature.end());

        // Add public key
        script_data.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA));
        uint32_t pk_len = public_key.size();
        script_data.insert(script_data.end(),
                          reinterpret_cast<uint8_t*>(&pk_len),
                          reinterpret_cast<uint8_t*>(&pk_len) + sizeof(pk_len));
        script_data.insert(script_data.end(), public_key.begin(), public_key.end());

        input.script_sig = Script(script_data);
    }

    return Result<Transaction>::Ok(signed_tx);
}

Result<uint256> Wallet::SendTransaction(const Transaction& tx, Blockchain& blockchain) {
    if (!impl_->is_loaded) {
        return Result<uint256>::Error("Wallet not loaded");
    }

    // Broadcast to network
    auto& mempool = blockchain.GetMempool();
    auto add_result = mempool.AddTransaction(tx);
    if (!add_result.IsOk()) {
        return Result<uint256>::Error("Failed to add to mempool: " + add_result.error);
    }

    // Save to wallet
    WalletTransaction wtx;
    wtx.txid = tx.GetHash();
    wtx.tx = tx;
    wtx.block_height = 0;
    wtx.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    impl_->db->WriteTransaction(wtx);
    impl_->transactions.push_back(wtx);

    return Result<uint256>::Ok(tx.GetHash());
}

Result<std::vector<TxOut>> Wallet::GetUTXOs() const {
    if (!impl_->is_loaded) {
        return Result<std::vector<TxOut>>::Error("Wallet not loaded");
    }

    std::vector<TxOut> utxos;
    for (const auto& [outpoint, txout] : impl_->utxos) {
        utxos.push_back(txout);
    }

    return Result<std::vector<TxOut>>::Ok(utxos);
}

Result<void> Wallet::UpdateUTXOs(Blockchain& blockchain) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    // Create a set of wallet addresses for fast lookup
    std::set<std::string> wallet_addresses;
    for (const auto& addr : impl_->addresses) {
        wallet_addresses.insert(addr.address);
    }

    // Clear existing UTXOs and transactions (will be rebuilt from blockchain)
    impl_->utxos.clear();
    impl_->transactions.clear();

    // Scan entire blockchain
    uint64_t height = blockchain.GetBestHeight();

    for (uint64_t h = 0; h <= height; h++) {
        auto block_result = blockchain.GetBlockByHeight(h);
        if (!block_result.IsOk()) {
            continue;  // Skip if block not found
        }

        const Block& block = block_result.value.value();
        uint64_t block_time = block.header.timestamp;

        // Process each transaction in the block
        for (size_t tx_idx = 0; tx_idx < block.transactions.size(); tx_idx++) {
            const Transaction& tx = block.transactions[tx_idx];
            uint256 txid = tx.GetHash();
            bool is_coinbase = (tx_idx == 0);
            bool is_wallet_tx = false;

            // Calculate amounts received and sent
            uint64_t amount_received = 0;
            uint64_t amount_sent = 0;

            // Check inputs (spent by us)
            for (const auto& input : tx.inputs) {
                if (!is_coinbase) {
                    // Find the previous output to check if it was ours
                    OutPoint prevout;
                    prevout.tx_hash = input.prev_tx_hash;
                    prevout.index = input.prev_tx_index;

                    // Check if this was a wallet UTXO being spent
                    auto utxo_it = impl_->utxos.find(prevout);
                    if (utxo_it != impl_->utxos.end()) {
                        amount_sent += utxo_it->second.value;
                        is_wallet_tx = true;
                        // Remove spent UTXO
                        impl_->utxos.erase(prevout);
                    }
                }
            }

            // Check outputs (received by us)
            for (size_t vout = 0; vout < tx.outputs.size(); vout++) {
                const TxOut& output = tx.outputs[vout];

                // Extract address from script
                std::string address = ExtractAddressFromScript(output.script_pubkey);

                if (wallet_addresses.count(address) > 0) {
                    // This output belongs to us
                    amount_received += output.value;
                    is_wallet_tx = true;

                    // Add to UTXOs
                    OutPoint outpoint;
                    outpoint.tx_hash = txid;
                    outpoint.index = vout;
                    impl_->utxos[outpoint] = output;
                }
            }

            // If this transaction involves our wallet, record it
            if (is_wallet_tx) {
                WalletTransaction wtx;
                wtx.txid = txid;
                wtx.tx = tx;
                wtx.block_height = h;
                wtx.timestamp = block_time;

                // Calculate net amount (received - sent)
                // For received transactions: amount_received > 0, amount_sent = 0
                // For sent transactions: amount_sent > amount_received (or equal if change)
                wtx.amount = static_cast<int64_t>(amount_received) - static_cast<int64_t>(amount_sent);

                // Calculate fee (only for transactions we sent)
                if (amount_sent > 0) {
                    uint64_t total_output = 0;
                    for (const auto& output : tx.outputs) {
                        total_output += output.value;
                    }
                    wtx.fee = amount_sent - total_output;
                } else {
                    wtx.fee = 0;
                }

                wtx.is_coinbase = is_coinbase;

                // Save to database
                auto write_result = impl_->db->WriteTransaction(wtx);
                if (write_result.IsOk()) {
                    impl_->transactions.push_back(wtx);
                }
            }
        }
    }

    return Result<void>::Ok();
}

Result<std::vector<std::string>> Wallet::GetMnemonic() const {
    if (!impl_->is_loaded) {
        return Result<std::vector<std::string>>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<std::vector<std::string>>::Error("Wallet is locked");
    }

    return Result<std::vector<std::string>>::Ok(impl_->mnemonic_words);
}

Result<void> Wallet::BackupWallet(const std::string& backup_path) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    return impl_->db->Backup(backup_path);
}

Result<void> Wallet::RestoreFromBackup(const std::string& backup_path) {
    // Verify backup path exists
    if (!DirectoryExists(backup_path)) {
        return Result<void>::Error("Backup directory does not exist: " + backup_path);
    }

    // Close wallet if loaded
    if (impl_->is_loaded) {
        auto close_result = Close();
        if (close_result.IsError()) {
            return Result<void>::Error("Failed to close wallet before restore: " + close_result.error);
        }
    }

    // Close database connection
    if (impl_->db) {
        impl_->db.reset();
    }

    // Create backup engine to access the backup
    rocksdb::BackupEngine* backup_engine = nullptr;
    rocksdb::BackupEngineOptions backup_options(backup_path);

    rocksdb::Status status = rocksdb::BackupEngine::Open(
        rocksdb::Env::Default(),
        backup_options,
        &backup_engine
    );

    if (!status.ok()) {
        return Result<void>::Error("Failed to open backup engine: " + status.ToString());
    }

    std::unique_ptr<rocksdb::BackupEngine> backup_ptr(backup_engine);

    // Get backup info
    std::vector<rocksdb::BackupInfo> backup_info;
    backup_ptr->GetBackupInfo(&backup_info);

    if (backup_info.empty()) {
        return Result<void>::Error("No backups found in: " + backup_path);
    }

    // Use the latest backup (highest backup_id)
    uint32_t latest_backup_id = backup_info.back().backup_id;

    // Restore database to original location
    // This will overwrite the current wallet database
    status = backup_ptr->RestoreDBFromBackup(
        latest_backup_id,
        impl_->config.data_dir,    // restore destination
        impl_->config.data_dir     // wal_dir (same as db_dir)
    );

    if (!status.ok()) {
        return Result<void>::Error("Failed to restore from backup: " + status.ToString());
    }

    // Reload the wallet from the restored database
    auto load_result = Load();
    if (load_result.IsError()) {
        return Result<void>::Error("Failed to reload wallet after restore: " + load_result.error);
    }

    return Result<void>::Ok();
}

Result<void> Wallet::Rescan(Blockchain& blockchain, uint64_t start_height) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    // Create a set of wallet addresses for fast lookup
    std::set<std::string> wallet_addresses;
    for (const auto& addr : impl_->addresses) {
        wallet_addresses.insert(addr.address);
    }

    // For correctness, clear and rebuild UTXOs from genesis
    // A proper implementation would track block heights per UTXO
    // and only remove/update those affected by the rescan range
    impl_->utxos.clear();

    // Scan from start_height to current height
    uint64_t height = blockchain.GetBestHeight();

    for (uint64_t h = start_height; h <= height; h++) {
        auto block_result = blockchain.GetBlockByHeight(h);
        if (!block_result.IsOk()) {
            continue;  // Skip if block not found
        }

        const Block& block = block_result.value.value();

        // Process each transaction in the block
        for (size_t tx_idx = 0; tx_idx < block.transactions.size(); tx_idx++) {
            const Transaction& tx = block.transactions[tx_idx];
            uint256 txid = tx.GetHash();

            // Remove spent UTXOs (process inputs)
            for (const auto& input : tx.inputs) {
                OutPoint prevout;
                prevout.tx_hash = input.prev_tx_hash;
                prevout.index = input.prev_tx_index;
                impl_->utxos.erase(prevout);
            }

            // Add new UTXOs (process outputs)
            for (size_t vout = 0; vout < tx.outputs.size(); vout++) {
                const TxOut& output = tx.outputs[vout];

                // Extract address from script
                std::string address = ExtractAddressFromScript(output.script_pubkey);

                if (wallet_addresses.count(address) > 0) {
                    // This output belongs to us - add to UTXOs
                    OutPoint outpoint;
                    outpoint.tx_hash = txid;
                    outpoint.index = vout;
                    impl_->utxos[outpoint] = output;
                }
            }
        }
    }

    return Result<void>::Ok();
}

Result<Wallet::WalletInfo> Wallet::GetInfo() const {
    if (!impl_->is_loaded) {
        return Result<WalletInfo>::Error("Wallet not loaded");
    }

    WalletInfo info;
    auto balance_result = GetBalance();
    info.balance = balance_result.IsOk() ? balance_result.value.value() : 0;

    auto unconfirmed_result = GetUnconfirmedBalance();
    info.unconfirmed_balance = unconfirmed_result.IsOk() ? unconfirmed_result.value.value() : 0;

    info.address_count = impl_->addresses.size();
    info.transaction_count = impl_->transactions.size();
    info.utxo_count = impl_->utxos.size();
    info.encrypted = impl_->is_encrypted;
    info.locked = impl_->is_locked;
    info.keypool_size = impl_->config.keypool_size;

    return Result<WalletInfo>::Ok(info);
}

// ============================================================================
// Wallet Transaction Builder Implementation
// ============================================================================

WalletTransactionBuilder::WalletTransactionBuilder(Wallet& wallet)
    : wallet_(wallet) {}

WalletTransactionBuilder& WalletTransactionBuilder::AddRecipient(const std::string& address,
                                                                 uint64_t amount) {
    recipients_.push_back({address, amount});
    return *this;
}

WalletTransactionBuilder& WalletTransactionBuilder::SetFeeRate(uint64_t fee_rate) {
    fee_rate_ = fee_rate;
    return *this;
}

WalletTransactionBuilder& WalletTransactionBuilder::SetComment(const std::string& comment) {
    comment_ = comment;
    return *this;
}

WalletTransactionBuilder& WalletTransactionBuilder::UseUTXOs(const std::vector<OutPoint>& utxos) {
    utxos_ = utxos;
    return *this;
}

Result<Transaction> WalletTransactionBuilder::BuildUnsigned() {
    return wallet_.CreateTransaction(recipients_, fee_rate_, comment_);
}

Result<Transaction> WalletTransactionBuilder::BuildAndSign() {
    auto tx_result = BuildUnsigned();
    if (!tx_result.IsOk()) {
        return tx_result;
    }

    return wallet_.SignTransaction(tx_result.value.value());
}

} // namespace wallet
} // namespace intcoin
