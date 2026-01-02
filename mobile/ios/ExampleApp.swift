// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

import SwiftUI

/// Example INTcoin wallet app for iOS
/// Demonstrates SDK usage with modern SwiftUI interface
@main
struct INTcoinWalletApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
        }
    }
}

struct ContentView: View {
    @StateObject private var walletViewModel = WalletViewModel()

    var body: some View {
        NavigationView {
            if walletViewModel.isWalletOpen {
                WalletView(viewModel: walletViewModel)
            } else {
                WalletSetupView(viewModel: walletViewModel)
            }
        }
    }
}

// MARK: - Wallet Setup View

struct WalletSetupView: View {
    @ObservedObject var viewModel: WalletViewModel
    @State private var password = ""
    @State private var confirmPassword = ""
    @State private var isCreatingWallet = true

    var body: some View {
        VStack(spacing: 30) {
            Image(systemName: "bitcoinsign.circle.fill")
                .font(.system(size: 80))
                .foregroundColor(.orange)

            Text("INTcoin Wallet")
                .font(.largeTitle)
                .fontWeight(.bold)

            Picker("Action", selection: $isCreatingWallet) {
                Text("Create Wallet").tag(true)
                Text("Open Wallet").tag(false)
            }
            .pickerStyle(.segmented)
            .padding(.horizontal)

            VStack(spacing: 15) {
                SecureField("Password", text: $password)
                    .textFieldStyle(.roundedBorder)
                    .padding(.horizontal)

                if isCreatingWallet {
                    SecureField("Confirm Password", text: $confirmPassword)
                        .textFieldStyle(.roundedBorder)
                        .padding(.horizontal)
                }
            }

            Button(action: {
                if isCreatingWallet {
                    viewModel.createWallet(password: password)
                } else {
                    viewModel.openWallet(password: password)
                }
            }) {
                Text(isCreatingWallet ? "Create Wallet" : "Open Wallet")
                    .fontWeight(.semibold)
                    .frame(maxWidth: .infinity)
                    .padding()
                    .background(Color.orange)
                    .foregroundColor(.white)
                    .cornerRadius(10)
            }
            .padding(.horizontal)
            .disabled(password.isEmpty || (isCreatingWallet && password != confirmPassword))

            if let error = viewModel.errorMessage {
                Text(error)
                    .foregroundColor(.red)
                    .font(.caption)
                    .padding(.horizontal)
            }

            Spacer()
        }
        .padding()
    }
}

// MARK: - Main Wallet View

struct WalletView: View {
    @ObservedObject var viewModel: WalletViewModel

    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                // Balance Card
                BalanceCard(balance: viewModel.balance)

                // Sync Status
                if viewModel.isSyncing {
                    SyncStatusView(progress: viewModel.syncProgress)
                }

                // Action Buttons
                HStack(spacing: 15) {
                    Button(action: { viewModel.showingReceive = true }) {
                        ActionButtonView(title: "Receive", icon: "arrow.down.circle.fill")
                    }

                    Button(action: { viewModel.showingSend = true }) {
                        ActionButtonView(title: "Send", icon: "arrow.up.circle.fill")
                    }
                }
                .padding(.horizontal)

                // Transaction History
                VStack(alignment: .leading, spacing: 10) {
                    Text("Recent Transactions")
                        .font(.headline)
                        .padding(.horizontal)

                    if viewModel.transactions.isEmpty {
                        Text("No transactions yet")
                            .foregroundColor(.gray)
                            .frame(maxWidth: .infinity, alignment: .center)
                            .padding()
                    } else {
                        ForEach(viewModel.transactions) { tx in
                            TransactionRow(transaction: tx)
                        }
                    }
                }
            }
            .padding(.vertical)
        }
        .navigationTitle("Wallet")
        .navigationBarTitleDisplayMode(.inline)
        .toolbar {
            ToolbarItem(placement: .navigationBarTrailing) {
                Button(action: {
                    if viewModel.isSyncing {
                        viewModel.stopSync()
                    } else {
                        viewModel.startSync()
                    }
                }) {
                    Image(systemName: viewModel.isSyncing ? "stop.circle" : "arrow.triangle.2.circlepath")
                }
            }
        }
        .sheet(isPresented: $viewModel.showingReceive) {
            ReceiveView(viewModel: viewModel)
        }
        .sheet(isPresented: $viewModel.showingSend) {
            SendView(viewModel: viewModel)
        }
    }
}

// MARK: - Balance Card

struct BalanceCard: View {
    let balance: Balance?

    var body: some View {
        VStack(spacing: 10) {
            Text("Total Balance")
                .font(.caption)
                .foregroundColor(.gray)

            if let balance = balance {
                Text(balance.totalFormatted)
                    .font(.system(size: 36, weight: .bold, design: .rounded))

                HStack(spacing: 20) {
                    VStack {
                        Text("Confirmed")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text(balance.confirmedFormatted)
                            .font(.caption)
                    }

                    VStack {
                        Text("Unconfirmed")
                            .font(.caption)
                            .foregroundColor(.gray)
                        Text(balance.unconfirmedFormatted)
                            .font(.caption)
                    }
                }
            } else {
                ProgressView()
            }
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(Color.orange.opacity(0.1))
        .cornerRadius(15)
        .padding(.horizontal)
    }
}

// MARK: - Sync Status View

struct SyncStatusView: View {
    let progress: Double

    var body: some View {
        VStack(spacing: 8) {
            Text("Syncing blockchain...")
                .font(.caption)
                .foregroundColor(.gray)

            ProgressView(value: progress, total: 1.0)
                .progressViewStyle(.linear)

            Text("\(Int(progress * 100))%")
                .font(.caption)
                .foregroundColor(.gray)
        }
        .padding()
        .background(Color.blue.opacity(0.1))
        .cornerRadius(10)
        .padding(.horizontal)
    }
}

// MARK: - Action Button View

struct ActionButtonView: View {
    let title: String
    let icon: String

    var body: some View {
        VStack {
            Image(systemName: icon)
                .font(.system(size: 30))
            Text(title)
                .font(.caption)
        }
        .frame(maxWidth: .infinity)
        .padding()
        .background(Color.orange)
        .foregroundColor(.white)
        .cornerRadius(10)
    }
}

// MARK: - Transaction Row

struct TransactionRow: View {
    let transaction: TransactionItem

    var body: some View {
        HStack {
            Image(systemName: transaction.isReceived ? "arrow.down.circle.fill" : "arrow.up.circle.fill")
                .foregroundColor(transaction.isReceived ? .green : .orange)

            VStack(alignment: .leading) {
                Text(transaction.address)
                    .font(.caption)
                    .lineLimit(1)
                Text(transaction.date, style: .date)
                    .font(.caption2)
                    .foregroundColor(.gray)
            }

            Spacer()

            VStack(alignment: .trailing) {
                Text(transaction.amountFormatted)
                    .fontWeight(.semibold)
                Text("\(transaction.confirmations) conf")
                    .font(.caption2)
                    .foregroundColor(.gray)
            }
        }
        .padding()
        .background(Color.gray.opacity(0.05))
        .cornerRadius(10)
        .padding(.horizontal)
    }
}

// MARK: - Receive View

struct ReceiveView: View {
    @ObservedObject var viewModel: WalletViewModel
    @Environment(\.dismiss) var dismiss

    var body: some View {
        NavigationView {
            VStack(spacing: 30) {
                Text("Scan to Receive INT")
                    .font(.headline)

                // QR Code placeholder (requires CoreImage)
                RoundedRectangle(cornerRadius: 10)
                    .fill(Color.gray.opacity(0.2))
                    .frame(width: 250, height: 250)
                    .overlay(
                        Text("QR Code")
                            .foregroundColor(.gray)
                    )

                if let address = viewModel.currentAddress {
                    Text(address)
                        .font(.system(.caption, design: .monospaced))
                        .padding()
                        .background(Color.gray.opacity(0.1))
                        .cornerRadius(8)

                    Button(action: {
                        UIPasteboard.general.string = address
                    }) {
                        Label("Copy Address", systemImage: "doc.on.doc")
                    }
                }

                Spacer()
            }
            .padding()
            .navigationTitle("Receive")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarTrailing) {
                    Button("Done") {
                        dismiss()
                    }
                }
            }
        }
    }
}

// MARK: - Send View

struct SendView: View {
    @ObservedObject var viewModel: WalletViewModel
    @Environment(\.dismiss) var dismiss
    @State private var recipientAddress = ""
    @State private var amount = ""

    var body: some View {
        NavigationView {
            Form {
                Section("Recipient") {
                    TextField("INT Address", text: $recipientAddress)
                        .autocapitalization(.none)
                        .font(.system(.body, design: .monospaced))

                    Button(action: {
                        // Scan QR code
                    }) {
                        Label("Scan QR Code", systemImage: "qrcode.viewfinder")
                    }
                }

                Section("Amount") {
                    TextField("0.00", text: $amount)
                        .keyboardType(.decimalPad)

                    Text("Available: \(viewModel.balance?.totalFormatted ?? "0.000000 INT")")
                        .font(.caption)
                        .foregroundColor(.gray)
                }

                Section {
                    Button(action: {
                        viewModel.sendTransaction(
                            toAddress: recipientAddress,
                            amount: amount
                        )
                        dismiss()
                    }) {
                        Text("Send INT")
                            .fontWeight(.semibold)
                            .frame(maxWidth: .infinity)
                    }
                    .disabled(!INTcoinSDK.validateAddress(recipientAddress) || amount.isEmpty)
                }
            }
            .navigationTitle("Send")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .navigationBarLeading) {
                    Button("Cancel") {
                        dismiss()
                    }
                }
            }
        }
    }
}

// MARK: - View Model

class WalletViewModel: ObservableObject {
    @Published var isWalletOpen = false
    @Published var balance: Balance?
    @Published var currentAddress: String?
    @Published var transactions: [TransactionItem] = []
    @Published var isSyncing = false
    @Published var syncProgress: Double = 0.0
    @Published var errorMessage: String?
    @Published var showingReceive = false
    @Published var showingSend = false

    private var sdk: INTcoinSDK?

    init() {
        do {
            sdk = try INTcoinSDK(network: "testnet")
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func createWallet(password: String) {
        guard let sdk = sdk else { return }

        do {
            let mnemonic = try sdk.createWallet(password: password)
            // TODO: Show mnemonic to user for backup
            print("Mnemonic: \(mnemonic)")
            isWalletOpen = true
            loadWalletData()
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func openWallet(password: String) {
        guard let sdk = sdk else { return }

        do {
            try sdk.openWallet(password: password)
            isWalletOpen = true
            loadWalletData()
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func loadWalletData() {
        guard let sdk = sdk else { return }

        do {
            balance = try sdk.getBalance()
            currentAddress = try sdk.getNewAddress()
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func startSync() {
        guard let sdk = sdk else { return }

        do {
            try sdk.startSync()
            isSyncing = true

            // Update progress periodically
            Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
                self.syncProgress = sdk.getSyncProgress()
                if self.syncProgress >= 1.0 {
                    self.isSyncing = false
                }
            }
        } catch {
            errorMessage = error.localizedDescription
        }
    }

    func stopSync() {
        sdk?.stopSync()
        isSyncing = false
    }

    func sendTransaction(toAddress: String, amount: String) {
        guard let sdk = sdk,
              let amountINTS = INTcoinSDK.parseINTAmount(amount) else {
            errorMessage = "Invalid amount"
            return
        }

        do {
            let txHash = try sdk.sendTransaction(toAddress: toAddress, amountINTS: amountINTS)
            print("Transaction sent: \(txHash.hex)")
            loadWalletData()  // Refresh balance
        } catch {
            errorMessage = error.localizedDescription
        }
    }
}

// MARK: - Data Models

struct TransactionItem: Identifiable {
    let id = UUID()
    let address: String
    let amountINTS: UInt64
    let isReceived: Bool
    let confirmations: UInt32
    let date: Date

    var amountFormatted: String {
        let formatted = INTcoinSDK.formatINTS(amountINTS)
        return isReceived ? "+\(formatted)" : "-\(formatted)"
    }
}

// MARK: - Extensions

extension Data {
    var hex: String {
        map { String(format: "%02x", $0) }.joined()
    }
}
