// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

import Foundation

/// INTcoin Mobile SDK for iOS
/// Provides native Swift interface for INTcoin lightweight wallet functionality
public class INTcoinSDK {

    // MARK: - Properties

    private var sdkHandle: OpaquePointer?
    private let network: String
    private let walletPath: String
    private let rpcEndpoint: String

    // Callbacks
    private var transactionCallback: ((TransactionEvent) -> Void)?
    private var syncProgressCallback: ((SyncProgress) -> Void)?

    // MARK: - Initialization

    /// Initialize INTcoin SDK
    /// - Parameters:
    ///   - network: Network type ("mainnet" or "testnet")
    ///   - walletPath: Path to wallet storage directory
    ///   - rpcEndpoint: RPC endpoint URL
    public init(network: String = "mainnet",
                walletPath: String? = nil,
                rpcEndpoint: String = "http://localhost:2210") throws {

        self.network = network
        self.rpcEndpoint = rpcEndpoint

        // Use app's documents directory if no path provided
        if let path = walletPath {
            self.walletPath = path
        } else {
            let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            self.walletPath = documentsPath.appendingPathComponent("intcoin_wallet").path
        }

        // Create SDK instance
        sdkHandle = intcoin_sdk_create(
            network.cString(using: .utf8),
            self.walletPath.cString(using: .utf8),
            rpcEndpoint.cString(using: .utf8)
        )

        guard sdkHandle != nil else {
            throw INTcoinError.initializationFailed
        }
    }

    deinit {
        if let handle = sdkHandle {
            intcoin_sdk_destroy(handle)
        }
    }

    // MARK: - Wallet Management

    /// Create new wallet with password encryption
    /// - Parameter password: Wallet encryption password
    /// - Returns: BIP39 mnemonic phrase (store safely!)
    public func createWallet(password: String) throws -> String {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        var mnemonicBuffer = [CChar](repeating: 0, count: 256)
        let result = intcoin_sdk_create_wallet(
            handle,
            password.cString(using: .utf8),
            &mnemonicBuffer
        )

        guard result == 0 else {
            throw INTcoinError.walletCreationFailed
        }

        return String(cString: mnemonicBuffer)
    }

    /// Open existing wallet
    /// - Parameter password: Wallet password
    public func openWallet(password: String) throws {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        let result = intcoin_sdk_open_wallet(handle, password.cString(using: .utf8))

        guard result == 0 else {
            throw INTcoinError.walletOpenFailed
        }
    }

    /// Close wallet
    public func closeWallet() {
        guard let handle = sdkHandle else { return }
        intcoin_sdk_close_wallet(handle)
    }

    // MARK: - Address Management

    /// Generate new receiving address
    /// - Returns: INTcoin Bech32 address
    public func getNewAddress() throws -> String {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        var addressBuffer = [CChar](repeating: 0, count: 64)
        let result = intcoin_sdk_get_new_address(handle, &addressBuffer)

        guard result == 0 else {
            throw INTcoinError.addressGenerationFailed
        }

        return String(cString: addressBuffer)
    }

    /// Validate INTcoin address format
    /// - Parameter address: Address to validate
    /// - Returns: True if valid
    public static func validateAddress(_ address: String) -> Bool {
        return intcoin_sdk_validate_address(address.cString(using: .utf8)) == 1
    }

    // MARK: - Balance & Transactions

    /// Get wallet balance
    /// - Returns: Wallet balance in INTS
    public func getBalance() throws -> Balance {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        var confirmed: UInt64 = 0
        var unconfirmed: UInt64 = 0
        let result = intcoin_sdk_get_balance(handle, &confirmed, &unconfirmed)

        guard result == 0 else {
            throw INTcoinError.balanceQueryFailed
        }

        return Balance(confirmed: confirmed, unconfirmed: unconfirmed)
    }

    /// Send transaction
    /// - Parameters:
    ///   - toAddress: Recipient address
    ///   - amountINTS: Amount in INTS (1 INT = 1,000,000 INTS)
    /// - Returns: Transaction hash
    public func sendTransaction(toAddress: String, amountINTS: UInt64) throws -> Data {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        var txHashBuffer = [UInt8](repeating: 0, count: 32)
        let result = intcoin_sdk_send_transaction(
            handle,
            toAddress.cString(using: .utf8),
            amountINTS,
            &txHashBuffer
        )

        guard result == 0 else {
            throw INTcoinError.transactionSendFailed
        }

        return Data(txHashBuffer)
    }

    // MARK: - Sync & Network

    /// Start blockchain sync
    public func startSync() throws {
        guard let handle = sdkHandle else {
            throw INTcoinError.sdkNotInitialized
        }

        let result = intcoin_sdk_start_sync(handle)

        guard result == 0 else {
            throw INTcoinError.syncStartFailed
        }
    }

    /// Stop blockchain sync
    public func stopSync() {
        guard let handle = sdkHandle else { return }
        intcoin_sdk_stop_sync(handle)
    }

    /// Get sync progress (0.0 to 1.0)
    /// - Returns: Sync progress as percentage
    public func getSyncProgress() -> Double {
        guard let handle = sdkHandle else { return 0.0 }
        return intcoin_sdk_get_sync_progress(handle)
    }

    // MARK: - QR Code Support

    /// Generate payment URI for QR code
    /// - Parameters:
    ///   - address: Receiving address
    ///   - amountINTS: Amount in INTS (optional, 0 for no amount)
    ///   - label: Payment label (optional)
    ///   - message: Payment message (optional)
    /// - Returns: intcoin: URI for QR code generation
    public static func generatePaymentURI(address: String,
                                         amountINTS: UInt64 = 0,
                                         label: String? = nil,
                                         message: String? = nil) -> String {
        var uriBuffer = [CChar](repeating: 0, count: 512)
        intcoin_sdk_generate_payment_uri(
            address.cString(using: .utf8),
            amountINTS,
            label?.cString(using: .utf8),
            message?.cString(using: .utf8),
            &uriBuffer
        )
        return String(cString: uriBuffer)
    }

    // MARK: - Utility

    /// Format INTS to human-readable string
    /// - Parameter ints: Amount in INTS
    /// - Returns: Formatted string (e.g., "1.234567 INT")
    public static func formatINTS(_ ints: UInt64) -> String {
        var buffer = [CChar](repeating: 0, count: 32)
        intcoin_sdk_format_ints(ints, &buffer)
        return String(cString: buffer)
    }

    /// Parse INT amount string to INTS
    /// - Parameter amountString: Amount string (e.g., "1.5" or "1500000")
    /// - Returns: Amount in INTS
    public static func parseINTAmount(_ amountString: String) -> UInt64? {
        // Parse "1.5" or "1500000" to INTS
        if let dotIndex = amountString.firstIndex(of: ".") {
            // Has decimal point, parse as INT
            let intPart = String(amountString[..<dotIndex])
            var fracPart = String(amountString[amountString.index(after: dotIndex)...])

            // Pad fraction to 6 digits
            while fracPart.count < 6 {
                fracPart += "0"
            }
            if fracPart.count > 6 {
                fracPart = String(fracPart.prefix(6))
            }

            guard let intValue = UInt64(intPart), let fracValue = UInt64(fracPart) else {
                return nil
            }

            return intValue * 1_000_000 + fracValue
        } else {
            // No decimal point, assume INTS
            return UInt64(amountString)
        }
    }

    // MARK: - Callbacks

    /// Set transaction event callback
    /// - Parameter callback: Callback function for transaction events
    public func setTransactionCallback(_ callback: @escaping (TransactionEvent) -> Void) {
        self.transactionCallback = callback
    }

    /// Set sync progress callback
    /// - Parameter callback: Callback function for sync progress updates
    public func setSyncProgressCallback(_ callback: @escaping (SyncProgress) -> Void) {
        self.syncProgressCallback = callback
    }
}

// MARK: - Data Types

/// Wallet balance
public struct Balance {
    /// Confirmed balance in INTS
    public let confirmed: UInt64

    /// Unconfirmed balance in INTS
    public let unconfirmed: UInt64

    /// Total balance (confirmed + unconfirmed) in INTS
    public var total: UInt64 {
        return confirmed + unconfirmed
    }

    /// Confirmed balance as formatted string
    public var confirmedFormatted: String {
        return INTcoinSDK.formatINTS(confirmed)
    }

    /// Unconfirmed balance as formatted string
    public var unconfirmedFormatted: String {
        return INTcoinSDK.formatINTS(unconfirmed)
    }

    /// Total balance as formatted string
    public var totalFormatted: String {
        return INTcoinSDK.formatINTS(total)
    }
}

/// Transaction event
public struct TransactionEvent {
    public enum EventType {
        case received, sent, confirmed, pending
    }

    public let type: EventType
    public let txHash: Data
    public let address: String
    public let amountINTS: UInt64
    public let confirmations: UInt32
    public let timestamp: Date
}

/// Sync progress
public struct SyncProgress {
    public let currentHeight: UInt64
    public let targetHeight: UInt64
    public let progress: Double  // 0.0 to 1.0
    public let isSyncing: Bool

    /// Progress as percentage (0-100)
    public var percentage: Double {
        return progress * 100.0
    }
}

/// INTcoin SDK errors
public enum INTcoinError: Error {
    case initializationFailed
    case sdkNotInitialized
    case walletCreationFailed
    case walletOpenFailed
    case addressGenerationFailed
    case balanceQueryFailed
    case transactionSendFailed
    case syncStartFailed
    case invalidAddress
    case insufficientBalance

    public var localizedDescription: String {
        switch self {
        case .initializationFailed:
            return "Failed to initialize INTcoin SDK"
        case .sdkNotInitialized:
            return "SDK not initialized"
        case .walletCreationFailed:
            return "Failed to create wallet"
        case .walletOpenFailed:
            return "Failed to open wallet"
        case .addressGenerationFailed:
            return "Failed to generate address"
        case .balanceQueryFailed:
            return "Failed to query balance"
        case .transactionSendFailed:
            return "Failed to send transaction"
        case .syncStartFailed:
            return "Failed to start sync"
        case .invalidAddress:
            return "Invalid INTcoin address"
        case .insufficientBalance:
            return "Insufficient balance"
        }
    }
}
