/*
 * Copyright (C) 2008-2012 The QXmpp developers
 *
 * Authors:
 *  Manjeet Dahiya
 *  Jeremy Lainé
 *
 * Source:
 *  http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include <QDomElement>
#include <QTextStream>
#include <QXmlStreamWriter>

#include "QXmppConstants.h"
#include "QXmppMessage.h"
#include "QXmppUtils.h"

static const char* chat_states[] = {
    "",
    "active",
    "inactive",
    "gone",
    "composing",
    "paused",
};

static const char* message_types[] = {
    "error",
    "normal",
    "chat",
    "groupchat",
    "headline"
};

static const char *ns_xhtml = "http://www.w3.org/1999/xhtml";

/// Constructs a QXmppMessage.
///
/// \param from
/// \param to
/// \param body
/// \param thread

QXmppMessage::QXmppMessage(const QString& from, const QString& to, const
                         QString& body, const QString& thread)
    : QXmppStanza(from, to),
      m_type(Chat),
      m_stampType(QXmppMessage::DelayedDelivery),
      m_state(None),
      m_attentionRequested(false),
      m_body(body),
      m_thread(thread),
      m_receiptRequested(false)
{
}

QXmppMessage::~QXmppMessage()
{

}

/// Returns the message's body.
///

QString QXmppMessage::body() const
{
    return m_body;
}

/// Sets the message's body.
///
/// \param body

void QXmppMessage::setBody(const QString& body)
{
    m_body = body;
}

/// Returns true if the user's attention is requested, as defined
/// by XEP-0224: Attention.

bool QXmppMessage::isAttentionRequested() const
{
    return m_attentionRequested;
}

/// Sets whether the user's attention is requested, as defined
/// by XEP-0224: Attention.
///
/// \a param requested

void QXmppMessage::setAttentionRequested(bool requested)
{
    m_attentionRequested = requested;
}

/// Returns true if a delivery receipt is requested, as defined
/// by XEP-0184: Message Delivery Receipts.

bool QXmppMessage::isReceiptRequested() const
{
    return m_receiptRequested;
}

/// Sets whether a delivery receipt is requested, as defined
/// by XEP-0184: Message Delivery Receipts.
///
/// \a param requested

void QXmppMessage::setReceiptRequested(bool requested)
{
    m_receiptRequested = requested;
    if (requested && id().isEmpty())
        generateAndSetNextId();
}

/// If this message is a delivery receipt, returns the ID of the
/// original message.

QString QXmppMessage::receiptId() const
{
    return m_receiptId;
}

/// Make this message a delivery receipt for the message with
/// the given \a id.

void QXmppMessage::setReceiptId(const QString &id)
{
    m_receiptId = id;
}

/// Returns the message's type.
///

QXmppMessage::Type QXmppMessage::type() const
{
    return m_type;
}

/// Sets the message's type.
///
/// \param type

void QXmppMessage::setType(QXmppMessage::Type type)
{
    m_type = type;
}

/// Returns the message's timestamp (if any).

QDateTime QXmppMessage::stamp() const
{
    return m_stamp;
}

/// Sets the message's timestamp.
///
/// \param stamp

void QXmppMessage::setStamp(const QDateTime &stamp)
{
    m_stamp = stamp;
}

/// Returns the message's chat state.
///

QXmppMessage::State QXmppMessage::state() const
{
    return m_state;
}

/// Sets the message's chat state.
///
/// \param state

void QXmppMessage::setState(QXmppMessage::State state)
{
    m_state = state;
}

/// Returns the message's subject.
///

QString QXmppMessage::subject() const
{
    return m_subject;
}

/// Sets the message's subject.
///
/// \param subject

void QXmppMessage::setSubject(const QString& subject)
{
    m_subject = subject;
}

/// Returns the message's thread.

QString QXmppMessage::thread() const
{
    return m_thread;
}

/// Sets the message's thread.
///
/// \param thread

void QXmppMessage::setThread(const QString& thread)
{
    m_thread = thread;
}

/// Returns the message's XHTML body as defined by
/// XEP-0071: XHTML-IM.

QString QXmppMessage::xhtml() const
{
    return m_xhtml;
}

/// Sets the message's XHTML body as defined by
/// XEP-0071: XHTML-IM.

void QXmppMessage::setXhtml(const QString &xhtml)
{
    m_xhtml = xhtml;
}

/// \cond
void QXmppMessage::parse(const QDomElement &element)
{
    QXmppStanza::parse(element);

    const QString type = element.attribute("type");
    m_type = Normal;
    for (int i = Error; i <= Headline; i++) {
        if (type == message_types[i]) {
            m_type = static_cast<Type>(i);
            break;
        }
    }

    m_body = element.firstChildElement("body").text();
    m_subject = element.firstChildElement("subject").text();
    m_thread = element.firstChildElement("thread").text();

    // chat states
    for (int i = Active; i <= Paused; i++)
    {
        QDomElement stateElement = element.firstChildElement(chat_states[i]);
        if (!stateElement.isNull() &&
            stateElement.namespaceURI() == ns_chat_states)
        {
            m_state = static_cast<QXmppMessage::State>(i);
            break;
        }
    }

    // XEP-0071: XHTML-IM
    QDomElement htmlElement = element.firstChildElement("html");
    if (!htmlElement.isNull() && htmlElement.namespaceURI() == ns_xhtml_im) {
        QDomElement bodyElement = htmlElement.firstChildElement("body");
        if (!bodyElement.isNull() && bodyElement.namespaceURI() == ns_xhtml) {
            QTextStream stream(&m_xhtml, QIODevice::WriteOnly);
            bodyElement.save(stream, 0);

            m_xhtml = m_xhtml.mid(m_xhtml.indexOf('>') + 1);
            m_xhtml.replace(" xmlns=\"http://www.w3.org/1999/xhtml\"", "");
            m_xhtml.replace("</body>", "");
            m_xhtml = m_xhtml.trimmed();
        }
    }

    // XEP-0184: Message Delivery Receipts
    QDomElement receivedElement = element.firstChildElement("received");
    if (!receivedElement.isNull() && receivedElement.namespaceURI() == ns_message_receipts) {
        m_receiptId = receivedElement.attribute("id");

        // compatibility with old-style XEP
        if (m_receiptId.isEmpty())
            m_receiptId = id();
    } else {
        m_receiptId = QString();
    }
    m_receiptRequested = element.firstChildElement("request").namespaceURI() == ns_message_receipts;

    // XEP-0203: Delayed Delivery
    QDomElement delayElement = element.firstChildElement("delay");
    if (!delayElement.isNull() && delayElement.namespaceURI() == ns_delayed_delivery)
    {
        const QString str = delayElement.attribute("stamp");
        m_stamp = QXmppUtils::datetimeFromString(str);
        m_stampType = QXmppMessage::DelayedDelivery;
    }

    // XEP-0224: Attention
    m_attentionRequested = element.firstChildElement("attention").namespaceURI() == ns_attention;

    QXmppElementList extensions;
    QDomElement xElement = element.firstChildElement("x");
    while (!xElement.isNull())
    {
        if (xElement.namespaceURI() == ns_legacy_delayed_delivery)
        {
            // XEP-0091: Legacy Delayed Delivery
            const QString str = xElement.attribute("stamp");
            m_stamp = QDateTime::fromString(str, "yyyyMMddThh:mm:ss");
            m_stamp.setTimeSpec(Qt::UTC);
            m_stampType = QXmppMessage::LegacyDelayedDelivery;
        } else {
            // other extensions
            extensions << QXmppElement(xElement);
        }
        xElement = xElement.nextSiblingElement("x");
    }
    setExtensions(extensions);
}

void QXmppMessage::toXml(QXmlStreamWriter *xmlWriter) const
{
    xmlWriter->writeStartElement("message");
    helperToXmlAddAttribute(xmlWriter, "xml:lang", lang());
    helperToXmlAddAttribute(xmlWriter, "id", id());
    helperToXmlAddAttribute(xmlWriter, "to", to());
    helperToXmlAddAttribute(xmlWriter, "from", from());
    helperToXmlAddAttribute(xmlWriter, "type", message_types[m_type]);
    if (!m_subject.isEmpty())
        helperToXmlAddTextElement(xmlWriter, "subject", m_subject);
    if (!m_body.isEmpty())
        helperToXmlAddTextElement(xmlWriter, "body", m_body);
    if (!m_thread.isEmpty())
        helperToXmlAddTextElement(xmlWriter, "thread", m_thread);
    error().toXml(xmlWriter);

    // chat states
    if (m_state > None && m_state <= Paused)
    {
        xmlWriter->writeStartElement(chat_states[m_state]);
        xmlWriter->writeAttribute("xmlns", ns_chat_states);
        xmlWriter->writeEndElement();
    }

    // XEP-0071: XHTML-IM
    if (!m_xhtml.isEmpty()) {
        xmlWriter->writeStartElement("html");
        xmlWriter->writeAttribute("xmlns", ns_xhtml_im);
        xmlWriter->writeStartElement("body");
        xmlWriter->writeAttribute("xmlns", ns_xhtml);
        xmlWriter->writeCharacters("");
        xmlWriter->device()->write(m_xhtml.toUtf8());
        xmlWriter->writeEndElement();
        xmlWriter->writeEndElement();
    }

    // time stamp
    if (m_stamp.isValid())
    {
        QDateTime utcStamp = m_stamp.toUTC();
        if (m_stampType == QXmppMessage::DelayedDelivery)
        {
            // XEP-0203: Delayed Delivery
            xmlWriter->writeStartElement("delay");
            xmlWriter->writeAttribute("xmlns", ns_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, "stamp", QXmppUtils::datetimeToString(utcStamp));
            xmlWriter->writeEndElement();
        } else {
            // XEP-0091: Legacy Delayed Delivery
            xmlWriter->writeStartElement("x");
            xmlWriter->writeAttribute("xmlns", ns_legacy_delayed_delivery);
            helperToXmlAddAttribute(xmlWriter, "stamp", utcStamp.toString("yyyyMMddThh:mm:ss"));
            xmlWriter->writeEndElement();
        }
    }

    // XEP-0184: Message Delivery Receipts
    if (!m_receiptId.isEmpty()) {
        xmlWriter->writeStartElement("received");
        xmlWriter->writeAttribute("xmlns", ns_message_receipts);
        xmlWriter->writeAttribute("id", m_receiptId);
        xmlWriter->writeEndElement();
    }
    if (m_receiptRequested) {
        xmlWriter->writeStartElement("request");
        xmlWriter->writeAttribute("xmlns", ns_message_receipts);
        xmlWriter->writeEndElement();
    }

    // XEP-0224: Attention
    if (m_attentionRequested) {
        xmlWriter->writeStartElement("attention");
        xmlWriter->writeAttribute("xmlns", ns_attention);
        xmlWriter->writeEndElement();
    }

    // other extensions
    foreach (const QXmppElement &extension, extensions())
        extension.toXml(xmlWriter);
    xmlWriter->writeEndElement();
}
/// \endcond
