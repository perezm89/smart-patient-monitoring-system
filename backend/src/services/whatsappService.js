
const twilio = require('twilio');

const WHATSAPP_COOLDOWN_MS = 30 * 1000; // 30 seconds for demo
const lastSentAt = new Map();
const pendingMessages = new Set();

const sendWhatsApp = async (message) => {
    if (process.env.WHATSAPP_ENABLED !== 'true') {
        console.log('[WhatsApp] Disabled');
        return { sent: false, reason: 'disabled' };
    }

    const to = process.env.TWILIO_WHATSAPP_TO;
    const from = process.env.TWILIO_WHATSAPP_FROM;

    if (!to || !from) {
        console.error('[WhatsApp] Missing sender or recipient');
        return { sent: false, reason: 'configuration' };
    }

    const cooldownKey = `${to}:${message.split(':')[0]}`;
    const now = Date.now();

    const lastSent = lastSentAt.get(cooldownKey);

    if (lastSent && now - lastSent < WHATSAPP_COOLDOWN_MS) {
        console.log('[WhatsApp] Cooldown active');
        return { sent: false, reason: 'cooldown' };
    }

    if (pendingMessages.has(cooldownKey)) {
        console.log('[WhatsApp] Message already pending');
        return { sent: false, reason: 'pending' };
    }

    pendingMessages.add(cooldownKey);

    try {
        const client = twilio(
            process.env.TWILIO_ACCOUNT_SID,
            process.env.TWILIO_AUTH_TOKEN
        );

        const result = await client.messages.create({
            body: message,
            from,
            to
        });

        lastSentAt.set(cooldownKey, Date.now());

        console.log(
            `[WhatsApp] Message submitted: ${result.sid}`
        );

        return { sent: true, sid: result.sid };

    } catch (error) {
        console.error(
            '[WhatsApp] Failed to send:',
            error.message
        );

        return { sent: false, reason: 'error' };

    } finally {
        pendingMessages.delete(cooldownKey);
    }
};

module.exports = { sendWhatsApp };
