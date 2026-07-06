#include <QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>

#include "models/messagemodel.h"

class TestMessageModel : public QObject
{
    Q_OBJECT

private slots:
    void parseMessageFields();
    void addMessagePrependsNewest();
    void updateMessageByTimestamp();
    void getLatestTimestamp();
    void groupingComputedOnLoad();
    void groupingRecomputedOnPrepend();
    void groupingRecomputedOnRemove();
    void addMessageDedupesByTimestamp();
    void applyReactionOptimistically();

private:
    QJsonObject messageJson(const QString &ts, const QString &userId,
                            const QString &text = QString("hello")) const;
    bool groupedAt(const MessageModel &model, int row) const;
};

QJsonObject TestMessageModel::messageJson(const QString &ts, const QString &userId,
                                          const QString &text) const
{
    QJsonObject json;
    json["ts"] = ts;
    json["user"] = userId;
    json["text"] = text;
    json["client_msg_id"] = QString("msg-%1").arg(ts);
    return json;
}

bool TestMessageModel::groupedAt(const MessageModel &model, int row) const
{
    return model.data(model.index(row, 0), MessageModel::IsGroupedWithPreviousRole).toBool();
}

void TestMessageModel::parseMessageFields()
{
    MessageModel model;
    model.setCurrentChannelId("C1");

    QJsonObject json = messageJson("1700000000.000100", "U1", "the text");
    json["thread_ts"] = "1699999999.000000";
    json["reply_count"] = 4;
    QJsonObject edited;
    edited["user"] = "U1";
    json["edited"] = edited;

    QJsonArray array;
    array.append(json);
    model.updateMessages(array);

    QCOMPARE(model.rowCount(), 1);
    QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx, MessageModel::TextRole).toString(), QString("the text"));
    QCOMPARE(model.data(idx, MessageModel::UserIdRole).toString(), QString("U1"));
    QCOMPARE(model.data(idx, MessageModel::TimestampRole).toString(), QString("1700000000.000100"));
    QCOMPARE(model.data(idx, MessageModel::ThreadTsRole).toString(), QString("1699999999.000000"));
    QCOMPARE(model.data(idx, MessageModel::ThreadCountRole).toInt(), 4);
    QCOMPARE(model.data(idx, MessageModel::IsEditedRole).toBool(), true);
    QCOMPARE(model.data(idx, MessageModel::ChannelIdRole).toString(), QString("C1"));
}

void TestMessageModel::addMessagePrependsNewest()
{
    MessageModel model;

    QJsonArray array;
    array.append(messageJson("1700000100.000000", "U1"));
    model.updateMessages(array);

    model.addMessage(messageJson("1700000200.000000", "U2"));

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.data(model.index(0, 0), MessageModel::UserIdRole).toString(), QString("U2"));
    QCOMPARE(model.data(model.index(1, 0), MessageModel::UserIdRole).toString(), QString("U1"));
}

void TestMessageModel::updateMessageByTimestamp()
{
    MessageModel model;

    QJsonArray array;
    array.append(messageJson("1700000200.000000", "U1", "original"));
    array.append(messageJson("1700000100.000000", "U2"));
    model.updateMessages(array);

    QJsonObject edited = messageJson("1700000200.000000", "U1", "changed");
    QJsonObject editedMarker;
    editedMarker["user"] = "U1";
    edited["edited"] = editedMarker;
    model.updateMessage(edited);

    QModelIndex idx = model.index(0, 0);
    QCOMPARE(model.data(idx, MessageModel::TextRole).toString(), QString("changed"));
    QCOMPARE(model.data(idx, MessageModel::IsEditedRole).toBool(), true);
    // The other message is untouched
    QCOMPARE(model.data(model.index(1, 0), MessageModel::UserIdRole).toString(), QString("U2"));
}

void TestMessageModel::getLatestTimestamp()
{
    MessageModel model;
    QCOMPARE(model.getLatestTimestamp(), QString());

    QJsonArray array;
    array.append(messageJson("1700000200.000000", "U1"));
    array.append(messageJson("1700000100.000000", "U1"));
    model.updateMessages(array);

    QCOMPARE(model.getLatestTimestamp(), QString("1700000200.000000"));
}

void TestMessageModel::groupingComputedOnLoad()
{
    MessageModel model;

    // Model order is newest first (as returned by conversations.history)
    QJsonArray array;
    array.append(messageJson("1700000400.000000", "U1"));  // row 0
    array.append(messageJson("1700000300.000000", "U1"));  // row 1: U1, 100s after row 0
    array.append(messageJson("1699999990.000000", "U1"));  // row 2: U1 but 310s gap
    array.append(messageJson("1699999900.000000", "U2"));  // row 3: other user
    model.updateMessages(array);

    QCOMPARE(groupedAt(model, 0), false);  // first row never grouped
    QCOMPARE(groupedAt(model, 1), true);   // same user, 100s apart
    QCOMPARE(groupedAt(model, 2), false);  // same user, gap > 300s
    QCOMPARE(groupedAt(model, 3), false);  // different user
}

void TestMessageModel::groupingRecomputedOnPrepend()
{
    MessageModel model;

    QJsonArray array;
    array.append(messageJson("1700000100.000000", "U1"));
    model.updateMessages(array);
    QCOMPARE(groupedAt(model, 0), false);

    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);

    // New message from the same user arrives 60s later
    model.addMessage(messageJson("1700000160.000000", "U1"));

    // The former row 0 (now row 1) becomes grouped with the new message
    QCOMPARE(groupedAt(model, 0), false);
    QCOMPARE(groupedAt(model, 1), true);
    QCOMPARE(dataSpy.count(), 1);
}

void TestMessageModel::groupingRecomputedOnRemove()
{
    MessageModel model;

    QJsonArray array;
    array.append(messageJson("1700000200.000000", "U1"));  // row 0
    array.append(messageJson("1700000100.000000", "U1"));  // row 1, grouped with row 0
    array.append(messageJson("1700000000.000000", "U2"));  // row 2
    model.updateMessages(array);
    QCOMPARE(groupedAt(model, 1), true);

    // Remove row 0; former row 1 moves up and is no longer grouped
    model.removeMessage("1700000200.000000");

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(groupedAt(model, 0), false);
}

void TestMessageModel::addMessageDedupesByTimestamp()
{
    MessageModel model;

    // Optimistic send + RTM echo deliver the same message twice
    model.addMessage(messageJson("1700000100.000000", "U1"));
    model.addMessage(messageJson("1700000100.000000", "U1"));

    QCOMPARE(model.rowCount(), 1);
}

void TestMessageModel::applyReactionOptimistically()
{
    MessageModel model;

    QJsonArray array;
    array.append(messageJson("1700000100.000000", "U1"));
    model.updateMessages(array);

    QModelIndex idx = model.index(0, 0);
    auto reactions = [&]() {
        return model.data(idx, MessageModel::ReactionsRole).toList();
    };

    // Add: creates the reaction bubble
    model.applyReaction("1700000100.000000", "thumbsup", "U_ME", true);
    QCOMPARE(reactions().count(), 1);
    QVariantMap reaction = reactions().first().toMap();
    QCOMPARE(reaction["name"].toString(), QString("thumbsup"));
    QCOMPARE(reaction["count"].toInt(), 1);

    // RTM echo of our own add is a no-op
    model.applyReaction("1700000100.000000", "thumbsup", "U_ME", true);
    QCOMPARE(reactions().first().toMap()["count"].toInt(), 1);

    // Another user piles on
    model.applyReaction("1700000100.000000", "thumbsup", "U_OTHER", true);
    QCOMPARE(reactions().first().toMap()["count"].toInt(), 2);

    // Removals
    model.applyReaction("1700000100.000000", "thumbsup", "U_ME", false);
    QCOMPARE(reactions().first().toMap()["count"].toInt(), 1);
    model.applyReaction("1700000100.000000", "thumbsup", "U_ME", false);  // echo, no-op
    QCOMPARE(reactions().first().toMap()["count"].toInt(), 1);
    model.applyReaction("1700000100.000000", "thumbsup", "U_OTHER", false);
    QCOMPARE(reactions().count(), 0);  // last user gone -> bubble removed
}

QTEST_GUILESS_MAIN(TestMessageModel)
#include "tst_messagemodel.moc"
