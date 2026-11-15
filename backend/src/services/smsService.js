
const twilio = require('twilio');

const smsEnabled = process.env.SMS_ENABLED === 'true';

const SMS_COOLDOWN_MS = 5 * 60 * 1000; // 5 minutes
const lastSentAt = new Map();
const pendingMessages = new Set();

const sendSMS = async (to, message) => {
    if (!smsEnabled) {
        console.log('[SMS] Disabled - message not sent');
        return { sent: false, reason: 'disabled' };
    }

    const cooldownKey = `${to}:${message.split(':')[0]}`;
    const now = Date.now();
    const lastSent = lastSentAt.get(cooldownKey);

    if (lastSent && now - lastSent < SMS_COOLDOWN_MS) {
        console.log('[SMS] Cooldown active - message skipped');
        return { sent: false, reason: 'cooldown' };
    }

    // Prevent simultaneous requests from sending duplicate SMS messages
    if (pendingMessages.has(cooldownKey)) {
        console.log('[SMS] Message already being processed - skipped');
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
            from: process.env.TWILIO_PHONE_NUMBER,
            to
        });

        // Activate cooldown only after Twilio accepts the request
        lastSentAt.set(cooldownKey, Date.now());

        console.log(`[SMS] Message submitted: ${result.sid}`);

        return { sent: true, sid: result.sid };

    } catch (error) {
        console.error('[SMS] Failed to send:', error.message);

        return { sent: false, reason: 'error' };

    } finally {
        // Always release the pending-message lock
        pendingMessages.delete(cooldownKey);
    }
};

module.exports = { sendSMS };
