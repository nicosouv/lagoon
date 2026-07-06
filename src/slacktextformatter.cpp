#include "slacktextformatter.h"
#include "models/usermodel.h"

#include <QHash>
#include <QRegularExpression>

#include <functional>

namespace {

QString replaceAll(const QString &input, const QRegularExpression &re,
                   const std::function<QString(const QRegularExpressionMatch &)> &replacer)
{
    QString result;
    int lastEnd = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(input);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        result += input.midRef(lastEnd, match.capturedStart() - lastEnd);
        result += replacer(match);
        lastEnd = match.capturedEnd();
    }
    result += input.midRef(lastEnd);
    return result;
}

// Slack emoji names -> Unicode, generated from EmojiHelper.js emojiMap
const QHash<QString, QString> &emojiMap()
{
    static QHash<QString, QString> map;
    if (map.isEmpty()) {
        map.insert(QStringLiteral("smile"), QString::fromUtf8("😄"));
        map.insert(QStringLiteral("smiley"), QString::fromUtf8("😃"));
        map.insert(QStringLiteral("grin"), QString::fromUtf8("😁"));
        map.insert(QStringLiteral("laughing"), QString::fromUtf8("😆"));
        map.insert(QStringLiteral("sweat_smile"), QString::fromUtf8("😅"));
        map.insert(QStringLiteral("joy"), QString::fromUtf8("😂"));
        map.insert(QStringLiteral("rofl"), QString::fromUtf8("🤣"));
        map.insert(QStringLiteral("relaxed"), QString::fromUtf8("☺️"));
        map.insert(QStringLiteral("blush"), QString::fromUtf8("😊"));
        map.insert(QStringLiteral("innocent"), QString::fromUtf8("😇"));
        map.insert(QStringLiteral("slightly_smiling_face"), QString::fromUtf8("🙂"));
        map.insert(QStringLiteral("upside_down_face"), QString::fromUtf8("🙃"));
        map.insert(QStringLiteral("wink"), QString::fromUtf8("😉"));
        map.insert(QStringLiteral("relieved"), QString::fromUtf8("😌"));
        map.insert(QStringLiteral("heart_eyes"), QString::fromUtf8("😍"));
        map.insert(QStringLiteral("kissing_heart"), QString::fromUtf8("😘"));
        map.insert(QStringLiteral("kissing"), QString::fromUtf8("😗"));
        map.insert(QStringLiteral("kissing_smiling_eyes"), QString::fromUtf8("😙"));
        map.insert(QStringLiteral("kissing_closed_eyes"), QString::fromUtf8("😚"));
        map.insert(QStringLiteral("yum"), QString::fromUtf8("😋"));
        map.insert(QStringLiteral("stuck_out_tongue"), QString::fromUtf8("😛"));
        map.insert(QStringLiteral("stuck_out_tongue_winking_eye"), QString::fromUtf8("😜"));
        map.insert(QStringLiteral("stuck_out_tongue_closed_eyes"), QString::fromUtf8("😝"));
        map.insert(QStringLiteral("neutral_face"), QString::fromUtf8("😐"));
        map.insert(QStringLiteral("expressionless"), QString::fromUtf8("😑"));
        map.insert(QStringLiteral("no_mouth"), QString::fromUtf8("😶"));
        map.insert(QStringLiteral("smirk"), QString::fromUtf8("😏"));
        map.insert(QStringLiteral("unamused"), QString::fromUtf8("😒"));
        map.insert(QStringLiteral("grimacing"), QString::fromUtf8("😬"));
        map.insert(QStringLiteral("lying_face"), QString::fromUtf8("🤥"));
        map.insert(QStringLiteral("pensive"), QString::fromUtf8("😔"));
        map.insert(QStringLiteral("sleepy"), QString::fromUtf8("😪"));
        map.insert(QStringLiteral("drooling_face"), QString::fromUtf8("🤤"));
        map.insert(QStringLiteral("sleeping"), QString::fromUtf8("😴"));
        map.insert(QStringLiteral("mask"), QString::fromUtf8("😷"));
        map.insert(QStringLiteral("face_with_thermometer"), QString::fromUtf8("🤒"));
        map.insert(QStringLiteral("face_with_head_bandage"), QString::fromUtf8("🤕"));
        map.insert(QStringLiteral("nauseated_face"), QString::fromUtf8("🤢"));
        map.insert(QStringLiteral("sneezing_face"), QString::fromUtf8("🤧"));
        map.insert(QStringLiteral("dizzy_face"), QString::fromUtf8("😵"));
        map.insert(QStringLiteral("zipper_mouth_face"), QString::fromUtf8("🤐"));
        map.insert(QStringLiteral("woozy_face"), QString::fromUtf8("🥴"));
        map.insert(QStringLiteral("exploding_head"), QString::fromUtf8("🤯"));
        map.insert(QStringLiteral("cowboy_hat_face"), QString::fromUtf8("🤠"));
        map.insert(QStringLiteral("partying_face"), QString::fromUtf8("🥳"));
        map.insert(QStringLiteral("sunglasses"), QString::fromUtf8("😎"));
        map.insert(QStringLiteral("nerd_face"), QString::fromUtf8("🤓"));
        map.insert(QStringLiteral("face_with_monocle"), QString::fromUtf8("🧐"));
        map.insert(QStringLiteral("confused"), QString::fromUtf8("😕"));
        map.insert(QStringLiteral("worried"), QString::fromUtf8("😟"));
        map.insert(QStringLiteral("slightly_frowning_face"), QString::fromUtf8("🙁"));
        map.insert(QStringLiteral("frowning_face"), QString::fromUtf8("☹️"));
        map.insert(QStringLiteral("open_mouth"), QString::fromUtf8("😮"));
        map.insert(QStringLiteral("hushed"), QString::fromUtf8("😯"));
        map.insert(QStringLiteral("astonished"), QString::fromUtf8("😲"));
        map.insert(QStringLiteral("flushed"), QString::fromUtf8("😳"));
        map.insert(QStringLiteral("pleading_face"), QString::fromUtf8("🥺"));
        map.insert(QStringLiteral("frowning"), QString::fromUtf8("😦"));
        map.insert(QStringLiteral("anguished"), QString::fromUtf8("😧"));
        map.insert(QStringLiteral("fearful"), QString::fromUtf8("😨"));
        map.insert(QStringLiteral("cold_sweat"), QString::fromUtf8("😰"));
        map.insert(QStringLiteral("disappointed_relieved"), QString::fromUtf8("😥"));
        map.insert(QStringLiteral("cry"), QString::fromUtf8("😢"));
        map.insert(QStringLiteral("sob"), QString::fromUtf8("😭"));
        map.insert(QStringLiteral("scream"), QString::fromUtf8("😱"));
        map.insert(QStringLiteral("confounded"), QString::fromUtf8("😖"));
        map.insert(QStringLiteral("persevere"), QString::fromUtf8("😣"));
        map.insert(QStringLiteral("disappointed"), QString::fromUtf8("😞"));
        map.insert(QStringLiteral("sweat"), QString::fromUtf8("😓"));
        map.insert(QStringLiteral("weary"), QString::fromUtf8("😩"));
        map.insert(QStringLiteral("tired_face"), QString::fromUtf8("😫"));
        map.insert(QStringLiteral("yawning_face"), QString::fromUtf8("🥱"));
        map.insert(QStringLiteral("triumph"), QString::fromUtf8("😤"));
        map.insert(QStringLiteral("rage"), QString::fromUtf8("😡"));
        map.insert(QStringLiteral("angry"), QString::fromUtf8("😠"));
        map.insert(QStringLiteral("smiling_imp"), QString::fromUtf8("😈"));
        map.insert(QStringLiteral("imp"), QString::fromUtf8("👿"));
        map.insert(QStringLiteral("skull"), QString::fromUtf8("💀"));
        map.insert(QStringLiteral("skull_and_crossbones"), QString::fromUtf8("☠️"));
        map.insert(QStringLiteral("wave"), QString::fromUtf8("👋"));
        map.insert(QStringLiteral("raised_back_of_hand"), QString::fromUtf8("🤚"));
        map.insert(QStringLiteral("raised_hand_with_fingers_splayed"), QString::fromUtf8("🖐️"));
        map.insert(QStringLiteral("hand"), QString::fromUtf8("✋"));
        map.insert(QStringLiteral("vulcan_salute"), QString::fromUtf8("🖖"));
        map.insert(QStringLiteral("ok_hand"), QString::fromUtf8("👌"));
        map.insert(QStringLiteral("pinching_hand"), QString::fromUtf8("🤏"));
        map.insert(QStringLiteral("v"), QString::fromUtf8("✌️"));
        map.insert(QStringLiteral("crossed_fingers"), QString::fromUtf8("🤞"));
        map.insert(QStringLiteral("love_you_gesture"), QString::fromUtf8("🤟"));
        map.insert(QStringLiteral("metal"), QString::fromUtf8("🤘"));
        map.insert(QStringLiteral("call_me_hand"), QString::fromUtf8("🤙"));
        map.insert(QStringLiteral("point_left"), QString::fromUtf8("👈"));
        map.insert(QStringLiteral("point_right"), QString::fromUtf8("👉"));
        map.insert(QStringLiteral("point_up_2"), QString::fromUtf8("👆"));
        map.insert(QStringLiteral("point_down"), QString::fromUtf8("👇"));
        map.insert(QStringLiteral("point_up"), QString::fromUtf8("☝️"));
        map.insert(QStringLiteral("+1"), QString::fromUtf8("👍"));
        map.insert(QStringLiteral("thumbsup"), QString::fromUtf8("👍"));
        map.insert(QStringLiteral("-1"), QString::fromUtf8("👎"));
        map.insert(QStringLiteral("thumbsdown"), QString::fromUtf8("👎"));
        map.insert(QStringLiteral("fist"), QString::fromUtf8("✊"));
        map.insert(QStringLiteral("facepunch"), QString::fromUtf8("👊"));
        map.insert(QStringLiteral("left-facing_fist"), QString::fromUtf8("🤛"));
        map.insert(QStringLiteral("right-facing_fist"), QString::fromUtf8("🤜"));
        map.insert(QStringLiteral("clap"), QString::fromUtf8("👏"));
        map.insert(QStringLiteral("raised_hands"), QString::fromUtf8("🙌"));
        map.insert(QStringLiteral("open_hands"), QString::fromUtf8("👐"));
        map.insert(QStringLiteral("palms_up_together"), QString::fromUtf8("🤲"));
        map.insert(QStringLiteral("handshake"), QString::fromUtf8("🤝"));
        map.insert(QStringLiteral("pray"), QString::fromUtf8("🙏"));
        map.insert(QStringLiteral("heart"), QString::fromUtf8("❤️"));
        map.insert(QStringLiteral("orange_heart"), QString::fromUtf8("🧡"));
        map.insert(QStringLiteral("yellow_heart"), QString::fromUtf8("💛"));
        map.insert(QStringLiteral("green_heart"), QString::fromUtf8("💚"));
        map.insert(QStringLiteral("blue_heart"), QString::fromUtf8("💙"));
        map.insert(QStringLiteral("purple_heart"), QString::fromUtf8("💜"));
        map.insert(QStringLiteral("black_heart"), QString::fromUtf8("🖤"));
        map.insert(QStringLiteral("brown_heart"), QString::fromUtf8("🤎"));
        map.insert(QStringLiteral("white_heart"), QString::fromUtf8("🤍"));
        map.insert(QStringLiteral("broken_heart"), QString::fromUtf8("💔"));
        map.insert(QStringLiteral("two_hearts"), QString::fromUtf8("💕"));
        map.insert(QStringLiteral("sparkling_heart"), QString::fromUtf8("💖"));
        map.insert(QStringLiteral("heartpulse"), QString::fromUtf8("💗"));
        map.insert(QStringLiteral("heartbeat"), QString::fromUtf8("💓"));
        map.insert(QStringLiteral("revolving_hearts"), QString::fromUtf8("💞"));
        map.insert(QStringLiteral("cupid"), QString::fromUtf8("💘"));
        map.insert(QStringLiteral("gift_heart"), QString::fromUtf8("💝"));
        map.insert(QStringLiteral("heart_decoration"), QString::fromUtf8("💟"));
        map.insert(QStringLiteral("peace_symbol"), QString::fromUtf8("☮️"));
        map.insert(QStringLiteral("star"), QString::fromUtf8("⭐"));
        map.insert(QStringLiteral("sparkles"), QString::fromUtf8("✨"));
        map.insert(QStringLiteral("fire"), QString::fromUtf8("🔥"));
        map.insert(QStringLiteral("100"), QString::fromUtf8("💯"));
        map.insert(QStringLiteral("zap"), QString::fromUtf8("⚡"));
        map.insert(QStringLiteral("boom"), QString::fromUtf8("💥"));
        map.insert(QStringLiteral("tada"), QString::fromUtf8("🎉"));
        map.insert(QStringLiteral("confetti_ball"), QString::fromUtf8("🎊"));
        map.insert(QStringLiteral("computer"), QString::fromUtf8("💻"));
        map.insert(QStringLiteral("keyboard"), QString::fromUtf8("⌨️"));
        map.insert(QStringLiteral("phone"), QString::fromUtf8("📱"));
        map.insert(QStringLiteral("email"), QString::fromUtf8("📧"));
        map.insert(QStringLiteral("memo"), QString::fromUtf8("📝"));
        map.insert(QStringLiteral("calendar"), QString::fromUtf8("📅"));
        map.insert(QStringLiteral("clock"), QString::fromUtf8("🕐"));
        map.insert(QStringLiteral("hourglass"), QString::fromUtf8("⌛"));
        map.insert(QStringLiteral("bulb"), QString::fromUtf8("💡"));
        map.insert(QStringLiteral("book"), QString::fromUtf8("📖"));
        map.insert(QStringLiteral("pencil2"), QString::fromUtf8("✏️"));
        map.insert(QStringLiteral("mag"), QString::fromUtf8("🔍"));
        map.insert(QStringLiteral("lock"), QString::fromUtf8("🔒"));
        map.insert(QStringLiteral("unlock"), QString::fromUtf8("🔓"));
        map.insert(QStringLiteral("key"), QString::fromUtf8("🔑"));
        map.insert(QStringLiteral("bell"), QString::fromUtf8("🔔"));
        map.insert(QStringLiteral("bookmark"), QString::fromUtf8("🔖"));
        map.insert(QStringLiteral("link"), QString::fromUtf8("🔗"));
        map.insert(QStringLiteral("paperclip"), QString::fromUtf8("📎"));
        map.insert(QStringLiteral("rocket"), QString::fromUtf8("🚀"));
        map.insert(QStringLiteral("airplane"), QString::fromUtf8("✈️"));
        map.insert(QStringLiteral("house"), QString::fromUtf8("🏠"));
        map.insert(QStringLiteral("sunny"), QString::fromUtf8("☀️"));
        map.insert(QStringLiteral("cloud"), QString::fromUtf8("☁️"));
        map.insert(QStringLiteral("umbrella"), QString::fromUtf8("☂️"));
        map.insert(QStringLiteral("snowflake"), QString::fromUtf8("❄️"));
        map.insert(QStringLiteral("rainbow"), QString::fromUtf8("🌈"));
        map.insert(QStringLiteral("tree"), QString::fromUtf8("🌳"));
        map.insert(QStringLiteral("herb"), QString::fromUtf8("🌿"));
        map.insert(QStringLiteral("four_leaf_clover"), QString::fromUtf8("🍀"));
        map.insert(QStringLiteral("seedling"), QString::fromUtf8("🌱"));
        map.insert(QStringLiteral("rose"), QString::fromUtf8("🌹"));
        map.insert(QStringLiteral("tulip"), QString::fromUtf8("🌷"));
        map.insert(QStringLiteral("bug"), QString::fromUtf8("🐛"));
        map.insert(QStringLiteral("bee"), QString::fromUtf8("🐝"));
        map.insert(QStringLiteral("cat"), QString::fromUtf8("🐱"));
        map.insert(QStringLiteral("dog"), QString::fromUtf8("🐶"));
        map.insert(QStringLiteral("mouse"), QString::fromUtf8("🐭"));
        map.insert(QStringLiteral("hamster"), QString::fromUtf8("🐹"));
        map.insert(QStringLiteral("rabbit"), QString::fromUtf8("🐰"));
        map.insert(QStringLiteral("fox_face"), QString::fromUtf8("🦊"));
        map.insert(QStringLiteral("bear"), QString::fromUtf8("🐻"));
        map.insert(QStringLiteral("panda_face"), QString::fromUtf8("🐼"));
        map.insert(QStringLiteral("apple"), QString::fromUtf8("🍎"));
        map.insert(QStringLiteral("banana"), QString::fromUtf8("🍌"));
        map.insert(QStringLiteral("grapes"), QString::fromUtf8("🍇"));
        map.insert(QStringLiteral("strawberry"), QString::fromUtf8("🍓"));
        map.insert(QStringLiteral("watermelon"), QString::fromUtf8("🍉"));
        map.insert(QStringLiteral("pizza"), QString::fromUtf8("🍕"));
        map.insert(QStringLiteral("hamburger"), QString::fromUtf8("🍔"));
        map.insert(QStringLiteral("fries"), QString::fromUtf8("🍟"));
        map.insert(QStringLiteral("hotdog"), QString::fromUtf8("🌭"));
        map.insert(QStringLiteral("taco"), QString::fromUtf8("🌮"));
        map.insert(QStringLiteral("burrito"), QString::fromUtf8("🌯"));
        map.insert(QStringLiteral("cake"), QString::fromUtf8("🍰"));
        map.insert(QStringLiteral("cookie"), QString::fromUtf8("🍪"));
        map.insert(QStringLiteral("ice_cream"), QString::fromUtf8("🍦"));
        map.insert(QStringLiteral("doughnut"), QString::fromUtf8("🍩"));
        map.insert(QStringLiteral("coffee"), QString::fromUtf8("☕"));
        map.insert(QStringLiteral("beer"), QString::fromUtf8("🍺"));
        map.insert(QStringLiteral("wine_glass"), QString::fromUtf8("🍷"));
        map.insert(QStringLiteral("cocktail"), QString::fromUtf8("🍸"));
        map.insert(QStringLiteral("flag-us"), QString::fromUtf8("🇺🇸"));
        map.insert(QStringLiteral("flag-gb"), QString::fromUtf8("🇬🇧"));
        map.insert(QStringLiteral("flag-fr"), QString::fromUtf8("🇫🇷"));
        map.insert(QStringLiteral("flag-de"), QString::fromUtf8("🇩🇪"));
        map.insert(QStringLiteral("flag-es"), QString::fromUtf8("🇪🇸"));
        map.insert(QStringLiteral("flag-it"), QString::fromUtf8("🇮🇹"));
        map.insert(QStringLiteral("flag-jp"), QString::fromUtf8("🇯🇵"));
        map.insert(QStringLiteral("flag-cn"), QString::fromUtf8("🇨🇳"));
        map.insert(QStringLiteral("flag-kr"), QString::fromUtf8("🇰🇷"));
        map.insert(QStringLiteral("flag-ca"), QString::fromUtf8("🇨🇦"));
        map.insert(QStringLiteral("flag-au"), QString::fromUtf8("🇦🇺"));
        map.insert(QStringLiteral("flag-br"), QString::fromUtf8("🇧🇷"));
        map.insert(QStringLiteral("flag-in"), QString::fromUtf8("🇮🇳"));
        map.insert(QStringLiteral("flag-ru"), QString::fromUtf8("🇷🇺"));
        map.insert(QStringLiteral("arrow_right"), QString::fromUtf8("➡️"));
        map.insert(QStringLiteral("arrow_left"), QString::fromUtf8("⬅️"));
        map.insert(QStringLiteral("arrow_up"), QString::fromUtf8("⬆️"));
        map.insert(QStringLiteral("arrow_down"), QString::fromUtf8("⬇️"));
        map.insert(QStringLiteral("white_check_mark"), QString::fromUtf8("✅"));
        map.insert(QStringLiteral("x"), QString::fromUtf8("❌"));
        map.insert(QStringLiteral("warning"), QString::fromUtf8("⚠️"));
        map.insert(QStringLiteral("bangbang"), QString::fromUtf8("‼️"));
        map.insert(QStringLiteral("question"), QString::fromUtf8("❓"));
        map.insert(QStringLiteral("information_source"), QString::fromUtf8("ℹ️"));
        map.insert(QStringLiteral("recycle"), QString::fromUtf8("♻️"));
        map.insert(QStringLiteral("copyright"), QString::fromUtf8("©️"));
        map.insert(QStringLiteral("registered"), QString::fromUtf8("®️"));
        map.insert(QStringLiteral("tm"), QString::fromUtf8("™️"));
    }
    return map;
}

} // namespace

QString SlackTextFormatter::convertEmoji(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }

    static const QRegularExpression emojiRe(QStringLiteral(":([a-z0-9_+-]+):"));
    return replaceAll(text, emojiRe, [](const QRegularExpressionMatch &match) {
        const QString emoji = emojiMap().value(match.captured(1));
        return emoji.isEmpty() ? match.captured(0) : emoji;
    });
}

QString SlackTextFormatter::format(const QString &input, const UserModel *userModel)
{
    if (input.isEmpty()) {
        return input;
    }

    QString text = input;

    // Escape HTML entities; '"' included so URLs cannot break out of href
    text.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    text.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    text.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    text.replace(QLatin1Char('"'), QLatin1String("&quot;"));

    // Slack links: <url|label> then <url>
    static const QRegularExpression labelledLink(QStringLiteral("&lt;(https?://[^|>]+)\\|([^>]+)&gt;"));
    text.replace(labelledLink, QStringLiteral("<a href=\"\\1\">\\2</a>"));
    static const QRegularExpression plainLink(QStringLiteral("&lt;(https?://[^>]+)&gt;"));
    text.replace(plainLink, QStringLiteral("<a href=\"\\1\">\\1</a>"));

    // Special mentions
    text.replace(QLatin1String("&lt;!channel&gt;"), QLatin1String("<b>@channel</b>"));
    text.replace(QLatin1String("&lt;!here&gt;"), QLatin1String("<b>@here</b>"));
    text.replace(QLatin1String("&lt;!everyone&gt;"), QLatin1String("<b>@everyone</b>"));

    // User mentions: <@USERID> -> @username
    static const QRegularExpression userMention(QStringLiteral("&lt;@([A-Z0-9]+)&gt;"));
    text = replaceAll(text, userMention, [userModel](const QRegularExpressionMatch &match) {
        const QString userId = match.captured(1);
        const QString name = userModel ? userModel->getUserName(userId) : userId;
        return QStringLiteral("<b>@%1</b>").arg(name.isEmpty() ? userId : name);
    });

    // Channel mentions: <#CHANNELID|name> or <#CHANNELID>
    static const QRegularExpression namedChannel(QStringLiteral("&lt;#[A-Z0-9]+\\|([^>]+)&gt;"));
    text.replace(namedChannel, QStringLiteral("<b>#\\1</b>"));
    static const QRegularExpression plainChannel(QStringLiteral("&lt;#([A-Z0-9]+)&gt;"));
    text.replace(plainChannel, QStringLiteral("<b>#channel</b>"));

    // Slack markdown
    static const QRegularExpression strike(QStringLiteral("~([^~]+)~"));
    text.replace(strike, QStringLiteral("<span style=\"text-decoration: line-through\">\\1</span>"));
    static const QRegularExpression bold(QStringLiteral("\\*([^*]+)\\*"));
    text.replace(bold, QStringLiteral("<b>\\1</b>"));
    static const QRegularExpression italic(QStringLiteral("(^|[\\s(])_([^_]+)_([\\s.,!?)]|$)"));
    text.replace(italic, QStringLiteral("\\1<i>\\2</i>\\3"));
    static const QRegularExpression code(QStringLiteral("`([^`]+)`"));
    text.replace(code, QStringLiteral("<span style=\"font-family: monospace; background-color: rgba(128,128,128,0.2); padding: 2px 4px; border-radius: 3px\">\\1</span>"));

    return convertEmoji(text);
}
