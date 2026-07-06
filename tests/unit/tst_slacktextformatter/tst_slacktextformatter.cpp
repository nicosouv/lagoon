#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>

#include "slacktextformatter.h"
#include "models/usermodel.h"

using namespace SlackTextFormatter;

class TestSlackTextFormatter : public QObject
{
    Q_OBJECT

private slots:
    void htmlIsEscaped();
    void quotesCannotEscapeHref();
    void labelledLink();
    void plainLink();
    void specialMentions();
    void userMentionWithModel();
    void userMentionWithoutModel();
    void channelMentions();
    void markdown();
    void emojiConversion();
    void emptyText();
};

void TestSlackTextFormatter::htmlIsEscaped()
{
    QCOMPARE(format("a <script> & \"quote\""),
             QString("a &lt;script&gt; &amp; &quot;quote&quot;"));
}

void TestSlackTextFormatter::quotesCannotEscapeHref()
{
    // 3.6: a quote inside the URL must not break out of the href attribute
    QString result = format("<https://x.test/a\"b|click>");
    QVERIFY(!result.contains("a\"b"));
    QCOMPARE(result, QString("<a href=\"https://x.test/a&quot;b\">click</a>"));
}

void TestSlackTextFormatter::labelledLink()
{
    QCOMPARE(format("see <https://example.org/page|the docs>!"),
             QString("see <a href=\"https://example.org/page\">the docs</a>!"));
}

void TestSlackTextFormatter::plainLink()
{
    QCOMPARE(format("<https://example.org>"),
             QString("<a href=\"https://example.org\">https://example.org</a>"));
}

void TestSlackTextFormatter::specialMentions()
{
    QCOMPARE(format("<!channel> <!here> <!everyone>"),
             QString("<b>@channel</b> <b>@here</b> <b>@everyone</b>"));
}

void TestSlackTextFormatter::userMentionWithModel()
{
    UserModel userModel;
    QJsonObject profile;
    profile["display_name"] = "Jane";
    QJsonObject user;
    user["id"] = "U42";
    user["name"] = "jane.doe";
    user["profile"] = profile;
    QJsonArray users;
    users.append(user);
    userModel.updateUsers(users);

    QCOMPARE(format("hey <@U42>!", &userModel), QString("hey <b>@Jane</b>!"));
}

void TestSlackTextFormatter::userMentionWithoutModel()
{
    QCOMPARE(format("hey <@U42>!"), QString("hey <b>@U42</b>!"));
}

void TestSlackTextFormatter::channelMentions()
{
    QCOMPARE(format("join <#C123|general>"), QString("join <b>#general</b>"));
    QCOMPARE(format("join <#C123>"), QString("join <b>#channel</b>"));
}

void TestSlackTextFormatter::markdown()
{
    QCOMPARE(format("*bold*"), QString("<b>bold</b>"));
    QCOMPARE(format("a _italic_ b"), QString("a <i>italic</i> b"));
    QCOMPARE(format("~gone~"),
             QString("<span style=\"text-decoration: line-through\">gone</span>"));
    QVERIFY(format("`code`").contains("font-family: monospace"));
    // Underscores inside words are not italics
    QCOMPARE(format("snake_case_name"), QString("snake_case_name"));
}

void TestSlackTextFormatter::emojiConversion()
{
    QCOMPARE(format("nice :thumbsup:"), QString::fromUtf8("nice 👍"));
    // Unknown emoji codes are left untouched
    QCOMPARE(format(":not_a_real_emoji_xyz:"), QString(":not_a_real_emoji_xyz:"));
}

void TestSlackTextFormatter::emptyText()
{
    QCOMPARE(format(QString()), QString());
}

QTEST_GUILESS_MAIN(TestSlackTextFormatter)
#include "tst_slacktextformatter.moc"
