/*
 * Copyright (c) 2025 INTcoin Team (Neil Adamson)
 * SPDX-License-Identifier: MIT License
 * Wallet Implementation - HD Wallet with BIP32/BIP39/BIP44
 */

#include "intcoin/wallet.h"
#include "intcoin/crypto.h"
#include "intcoin/util.h"
#include "intcoin/blockchain.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <fstream>
#include <chrono>

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

    // First 32 bytes = private key (Dilithium3 seed)
    SecretKey sk;
    if (hmac.size() >= DILITHIUM3_SECRETKEYBYTES) {
        std::copy(hmac.begin(), hmac.begin() + DILITHIUM3_SECRETKEYBYTES, sk.begin());
    } else {
        // If HMAC output is smaller, use it as seed
        std::copy(hmac.begin(), hmac.end(), sk.begin());
    }
    master.private_key = sk;

    // Derive public key from private key
    auto pk_result = Dilithium3Crypto::GetPublicKey(sk);
    if (!pk_result.IsOk()) {
        return Result<ExtendedKey>::Error("Failed to derive public key");
    }
    master.public_key = pk_result.value.value();

    // Chain code from hash
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
        SecretKey child_sk;
        // For quantum-resistant keys, we use the HMAC output as child key seed
        // (simplified - production would use proper key derivation)
        if (hmac.size() >= DILITHIUM3_SECRETKEYBYTES) {
            std::copy(hmac.begin(), hmac.begin() + DILITHIUM3_SECRETKEYBYTES, child_sk.begin());
        } else {
            std::copy(hmac.begin(), hmac.end(), child_sk.begin());
        }
        child.private_key = child_sk;

        // Derive public key
        auto pk_result = Dilithium3Crypto::GetPublicKey(child_sk);
        if (!pk_result.IsOk()) {
            return Result<ExtendedKey>::Error("Failed to derive child public key");
        }
        child.public_key = pk_result.value.value();
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
    // TODO: Implement base58check serialization
    // For now, return placeholder
    return "xprv_placeholder";
}

Result<ExtendedKey> ExtendedKey::DeserializeBase58(const std::string& str) {
    // TODO: Implement base58check deserialization
    return Result<ExtendedKey>::Error("Not implemented");
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
    std::unique_ptr<BlockchainDB> db;
    bool is_open = false;

    explicit Impl(const std::string& path) : wallet_path(path) {}
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

    DBConfig config;
    config.path = impl_->wallet_path;
    impl_->db = std::make_unique<BlockchainDB>(config);

    auto result = impl_->db->Open();
    if (!result.IsOk()) {
        return result;
    }

    impl_->is_open = true;
    return Result<void>::Ok();
}

Result<void> WalletDB::Close() {
    if (!impl_->is_open) {
        return Result<void>::Error("Wallet database not open");
    }

    auto result = impl_->db->Close();
    if (!result.IsOk()) {
        return result;
    }

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

    // Serialize address data
    std::vector<uint8_t> data;
    // TODO: Proper serialization

    std::string key = "addr_" + addr.address;
    return impl_->db->Put(key, data);
}

Result<WalletAddress> WalletDB::ReadAddress(const std::string& address) {
    if (!impl_->is_open) {
        return Result<WalletAddress>::Error("Database not open");
    }

    std::string key = "addr_" + address;
    auto result = impl_->db->Get(key);
    if (!result.IsOk()) {
        return Result<WalletAddress>::Error("Address not found");
    }

    // TODO: Deserialize address data
    WalletAddress addr;
    addr.address = address;
    return Result<WalletAddress>::Ok(addr);
}

Result<std::vector<WalletAddress>> WalletDB::ReadAllAddresses() {
    if (!impl_->is_open) {
        return Result<std::vector<WalletAddress>>::Error("Database not open");
    }

    // TODO: Iterate through all addresses
    std::vector<WalletAddress> addresses;
    return Result<std::vector<WalletAddress>>::Ok(addresses);
}

Result<void> WalletDB::DeleteAddress(const std::string& address) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "addr_" + address;
    return impl_->db->Delete(key);
}

Result<void> WalletDB::WriteTransaction(const WalletTransaction& wtx) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "tx_" + Uint256ToHex(wtx.txid);
    auto serialized = wtx.tx.Serialize();
    return impl_->db->Put(key, serialized);
}

Result<WalletTransaction> WalletDB::ReadTransaction(const uint256& txid) {
    if (!impl_->is_open) {
        return Result<WalletTransaction>::Error("Database not open");
    }

    std::string key = "tx_" + Uint256ToHex(txid);
    auto result = impl_->db->Get(key);
    if (!result.IsOk()) {
        return Result<WalletTransaction>::Error("Transaction not found");
    }

    auto tx_result = Transaction::Deserialize(result.value.value());
    if (!tx_result.IsOk()) {
        return Result<WalletTransaction>::Error("Failed to deserialize transaction");
    }

    WalletTransaction wtx;
    wtx.txid = txid;
    wtx.tx = tx_result.value.value();
    return Result<WalletTransaction>::Ok(wtx);
}

Result<std::vector<WalletTransaction>> WalletDB::ReadAllTransactions() {
    if (!impl_->is_open) {
        return Result<std::vector<WalletTransaction>>::Error("Database not open");
    }

    // TODO: Iterate through all transactions
    std::vector<WalletTransaction> transactions;
    return Result<std::vector<WalletTransaction>>::Ok(transactions);
}

Result<void> WalletDB::DeleteTransaction(const uint256& txid) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "tx_" + Uint256ToHex(txid);
    return impl_->db->Delete(key);
}

Result<void> WalletDB::WriteMasterKey(const std::vector<uint8_t>& encrypted_seed) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    return impl_->db->Put("master_key", encrypted_seed);
}

Result<std::vector<uint8_t>> WalletDB::ReadMasterKey() {
    if (!impl_->is_open) {
        return Result<std::vector<uint8_t>>::Error("Database not open");
    }

    return impl_->db->Get("master_key");
}

Result<void> WalletDB::WriteMetadata(const std::string& key, const std::string& value) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string db_key = "meta_" + key;
    std::vector<uint8_t> data(value.begin(), value.end());
    return impl_->db->Put(db_key, data);
}

Result<std::string> WalletDB::ReadMetadata(const std::string& key) {
    if (!impl_->is_open) {
        return Result<std::string>::Error("Database not open");
    }

    std::string db_key = "meta_" + key;
    auto result = impl_->db->Get(db_key);
    if (!result.IsOk()) {
        return Result<std::string>::Error("Metadata not found");
    }

    std::string value(result.value.value().begin(), result.value.value().end());
    return Result<std::string>::Ok(value);
}

Result<void> WalletDB::WriteLabel(const std::string& address, const std::string& label) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    std::string key = "label_" + address;
    std::vector<uint8_t> data(label.begin(), label.end());
    return impl_->db->Put(key, data);
}

Result<std::string> WalletDB::ReadLabel(const std::string& address) {
    if (!impl_->is_open) {
        return Result<std::string>::Error("Database not open");
    }

    std::string key = "label_" + address;
    auto result = impl_->db->Get(key);
    if (!result.IsOk()) {
        return Result<std::string>::Error("Label not found");
    }

    std::string label(result.value.value().begin(), result.value.value().end());
    return Result<std::string>::Ok(label);
}

Result<void> WalletDB::Backup(const std::string& backup_path) {
    if (!impl_->is_open) {
        return Result<void>::Error("Database not open");
    }

    // TODO: Implement database backup
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

    // TODO: Implement encryption using Kyber768 or AES-256
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

    // TODO: Verify passphrase and decrypt master key
    impl_->is_locked = false;

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

    // TODO: Verify old passphrase and re-encrypt with new passphrase

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

    // TODO: Calculate unconfirmed balance from mempool transactions
    return Result<uint64_t>::Ok(0);
}

Result<uint64_t> Wallet::GetAddressBalance(const std::string& address) const {
    if (!impl_->is_loaded) {
        return Result<uint64_t>::Error("Wallet not loaded");
    }

    // TODO: Calculate balance for specific address
    return Result<uint64_t>::Ok(0);
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
                                              const std::string& comment) {
    if (!impl_->is_loaded) {
        return Result<Transaction>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<Transaction>::Error("Wallet is locked");
    }

    // TODO: Implement transaction creation
    // - Select UTXOs (coin selection)
    // - Create inputs and outputs
    // - Calculate fee
    // - Add change output if needed

    return Result<Transaction>::Error("Not implemented yet");
}

Result<Transaction> Wallet::SignTransaction(const Transaction& tx) {
    if (!impl_->is_loaded) {
        return Result<Transaction>::Error("Wallet not loaded");
    }

    if (impl_->is_locked) {
        return Result<Transaction>::Error("Wallet is locked");
    }

    // TODO: Implement transaction signing
    // - For each input, find corresponding private key
    // - Sign transaction with Dilithium3

    return Result<Transaction>::Error("Not implemented yet");
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

    // TODO: Scan blockchain for UTXOs belonging to wallet addresses
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
    // TODO: Implement restore from backup
    return Result<void>::Error("Not implemented yet");
}

Result<void> Wallet::Rescan(Blockchain& blockchain, uint64_t start_height) {
    if (!impl_->is_loaded) {
        return Result<void>::Error("Wallet not loaded");
    }

    // TODO: Rescan blockchain for wallet transactions
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
