// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

package org.intcoin.sdk

import android.content.Context
import java.io.File

/**
 * INTcoin Mobile SDK for Android
 * Provides native Kotlin interface for INTcoin lightweight wallet functionality
 */
class INTcoinSDK private constructor(
    private val network: String,
    private val walletPath: String,
    private val rpcEndpoint: String
) : AutoCloseable {

    private var sdkHandle: Long = 0
    private var transactionCallback: ((TransactionEvent) -> Unit)? = null
    private var syncProgressCallback: ((SyncProgress) -> Unit)? = null

    init {
        // Load native library
        System.loadLibrary("intcoin_core")

        // Initialize SDK
        sdkHandle = nativeCreate(network, walletPath, rpcEndpoint)
        if (sdkHandle == 0L) {
            throw INTcoinException("Failed to initialize INTcoin SDK")
        }
    }

    override fun close() {
        if (sdkHandle != 0L) {
            nativeDestroy(sdkHandle)
            sdkHandle = 0
        }
    }

    // MARK: - Wallet Management

    /**
     * Create new wallet with password encryption
     * @param password Wallet encryption password
     * @return BIP39 mnemonic phrase (store safely!)
     */
    @Throws(INTcoinException::class)
    fun createWallet(password: String): String {
        checkHandle()
        return nativeCreateWallet(sdkHandle, password)
            ?: throw INTcoinException("Failed to create wallet")
    }

    /**
     * Open existing wallet
     * @param password Wallet password
     */
    @Throws(INTcoinException::class)
    fun openWallet(password: String) {
        checkHandle()
        if (!nativeOpenWallet(sdkHandle, password)) {
            throw INTcoinException("Failed to open wallet - incorrect password?")
        }
    }

    /**
     * Close wallet
     */
    fun closeWallet() {
        if (sdkHandle != 0L) {
            nativeCloseWallet(sdkHandle)
        }
    }

    // MARK: - Address Management

    /**
     * Generate new receiving address
     * @return INTcoin Bech32 address
     */
    @Throws(INTcoinException::class)
    fun getNewAddress(): String {
        checkHandle()
        return nativeGetNewAddress(sdkHandle)
            ?: throw INTcoinException("Failed to generate address")
    }

    // MARK: - Balance & Transactions

    /**
     * Get wallet balance
     * @return Wallet balance in INTS
     */
    @Throws(INTcoinException::class)
    fun getBalance(): Balance {
        checkHandle()
        return nativeGetBalance(sdkHandle)
            ?: throw INTcoinException("Failed to query balance")
    }

    /**
     * Send transaction
     * @param toAddress Recipient address
     * @param amountINTS Amount in INTS (1 INT = 1,000,000 INTS)
     * @return Transaction hash as byte array
     */
    @Throws(INTcoinException::class)
    fun sendTransaction(toAddress: String, amountINTS: Long): ByteArray {
        checkHandle()
        if (!validateAddress(toAddress)) {
            throw INTcoinException("Invalid recipient address")
        }
        return nativeSendTransaction(sdkHandle, toAddress, amountINTS)
            ?: throw INTcoinException("Failed to send transaction")
    }

    // MARK: - Sync & Network

    /**
     * Start blockchain sync
     */
    @Throws(INTcoinException::class)
    fun startSync() {
        checkHandle()
        if (!nativeStartSync(sdkHandle)) {
            throw INTcoinException("Failed to start sync")
        }
    }

    /**
     * Stop blockchain sync
     */
    fun stopSync() {
        if (sdkHandle != 0L) {
            nativeStopSync(sdkHandle)
        }
    }

    /**
     * Get sync progress (0.0 to 1.0)
     * @return Sync progress as fraction
     */
    fun getSyncProgress(): Double {
        return if (sdkHandle != 0L) {
            nativeGetSyncProgress(sdkHandle)
        } else {
            0.0
        }
    }

    // MARK: - Callbacks

    /**
     * Set transaction event callback
     * @param callback Function to call on transaction events
     */
    fun setTransactionCallback(callback: (TransactionEvent) -> Unit) {
        this.transactionCallback = callback
    }

    /**
     * Set sync progress callback
     * @param callback Function to call on sync progress updates
     */
    fun setSyncProgressCallback(callback: (SyncProgress) -> Unit) {
        this.syncProgressCallback = callback
    }

    // MARK: - Private Methods

    private fun checkHandle() {
        if (sdkHandle == 0L) {
            throw INTcoinException("SDK not initialized")
        }
    }

    // MARK: - Native Methods

    private external fun nativeCreate(network: String, walletPath: String, rpcEndpoint: String): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeCreateWallet(handle: Long, password: String): String?
    private external fun nativeOpenWallet(handle: Long, password: String): Boolean
    private external fun nativeCloseWallet(handle: Long)
    private external fun nativeGetNewAddress(handle: Long): String?
    private external fun nativeGetBalance(handle: Long): Balance?
    private external fun nativeSendTransaction(handle: Long, toAddress: String, amountINTS: Long): ByteArray?
    private external fun nativeStartSync(handle: Long): Boolean
    private external fun nativeStopSync(handle: Long)
    private external fun nativeGetSyncProgress(handle: Long): Double

    companion object {
        /**
         * Create SDK instance
         * @param context Android context
         * @param network Network type ("mainnet" or "testnet")
         * @param rpcEndpoint RPC endpoint URL
         * @return SDK instance
         */
        @JvmStatic
        fun create(
            context: Context,
            network: String = "mainnet",
            rpcEndpoint: String = "http://localhost:2210"
        ): INTcoinSDK {
            // Use app's files directory for wallet storage
            val walletPath = File(context.filesDir, "intcoin_wallet").absolutePath

            return INTcoinSDK(network, walletPath, rpcEndpoint)
        }

        /**
         * Validate INTcoin address format
         * @param address Address to validate
         * @return True if valid
         */
        @JvmStatic
        fun validateAddress(address: String): Boolean {
            return nativeValidateAddress(address)
        }

        /**
         * Format INTS to human-readable string
         * @param ints Amount in INTS
         * @return Formatted string (e.g., "1.234567 INT")
         */
        @JvmStatic
        fun formatINTS(ints: Long): String {
            return nativeFormatINTS(ints)
        }

        /**
         * Parse INT amount string to INTS
         * @param amountString Amount string (e.g., "1.5" or "1500000")
         * @return Amount in INTS, or null if invalid
         */
        @JvmStatic
        fun parseINTAmount(amountString: String): Long? {
            val dotIndex = amountString.indexOf('.')

            return if (dotIndex == -1) {
                // No decimal point, assume INTS
                amountString.toLongOrNull()
            } else {
                // Has decimal point, parse as INT
                try {
                    val intPart = amountString.substring(0, dotIndex)
                    var fracPart = amountString.substring(dotIndex + 1)

                    // Pad fraction to 6 digits
                    while (fracPart.length < 6) {
                        fracPart += "0"
                    }
                    if (fracPart.length > 6) {
                        fracPart = fracPart.substring(0, 6)
                    }

                    val intValue = intPart.toLong()
                    val fracValue = fracPart.toLong()

                    intValue * 1_000_000 + fracValue
                } catch (e: Exception) {
                    null
                }
            }
        }

        /**
         * Generate payment URI for QR code
         * @param address Receiving address
         * @param amountINTS Amount in INTS (0 for no amount)
         * @param label Payment label (optional)
         * @param message Payment message (optional)
         * @return intcoin: URI for QR code generation
         */
        @JvmStatic
        fun generatePaymentURI(
            address: String,
            amountINTS: Long = 0,
            label: String? = null,
            message: String? = null
        ): String {
            return nativeGeneratePaymentURI(address, amountINTS, label, message)
        }

        // Native companion methods
        @JvmStatic
        private external fun nativeValidateAddress(address: String): Boolean

        @JvmStatic
        private external fun nativeFormatINTS(ints: Long): String

        @JvmStatic
        private external fun nativeGeneratePaymentURI(
            address: String,
            amountINTS: Long,
            label: String?,
            message: String?
        ): String
    }
}

// MARK: - Data Classes

/**
 * Wallet balance
 */
data class Balance(
    /** Confirmed balance in INTS */
    val confirmed: Long,

    /** Unconfirmed balance in INTS */
    val unconfirmed: Long
) {
    /** Total balance (confirmed + unconfirmed) in INTS */
    val total: Long
        get() = confirmed + unconfirmed

    /** Confirmed balance as formatted string */
    val confirmedFormatted: String
        get() = INTcoinSDK.formatINTS(confirmed)

    /** Unconfirmed balance as formatted string */
    val unconfirmedFormatted: String
        get() = INTcoinSDK.formatINTS(unconfirmed)

    /** Total balance as formatted string */
    val totalFormatted: String
        get() = INTcoinSDK.formatINTS(total)
}

/**
 * Transaction event
 */
data class TransactionEvent(
    val type: Type,
    val txHash: ByteArray,
    val address: String,
    val amountINTS: Long,
    val confirmations: Int,
    val timestamp: Long
) {
    enum class Type {
        RECEIVED, SENT, CONFIRMED, PENDING
    }

    val amountFormatted: String
        get() = INTcoinSDK.formatINTS(amountINTS)

    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as TransactionEvent

        if (!txHash.contentEquals(other.txHash)) return false

        return true
    }

    override fun hashCode(): Int {
        return txHash.contentHashCode()
    }
}

/**
 * Sync progress
 */
data class SyncProgress(
    val currentHeight: Long,
    val targetHeight: Long,
    val progress: Double,  // 0.0 to 1.0
    val isSyncing: Boolean
) {
    /** Progress as percentage (0-100) */
    val percentage: Double
        get() = progress * 100.0
}

/**
 * INTcoin SDK exception
 */
class INTcoinException(message: String) : Exception(message)

// MARK: - Extensions

/**
 * Convert byte array to hex string
 */
fun ByteArray.toHex(): String {
    return joinToString("") { "%02x".format(it) }
}
