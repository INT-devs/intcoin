// swift-tools-version:5.9
// Copyright (c) 2024-2025 The INTcoin Core developers
// Distributed under the MIT software license

import PackageDescription

let package = Package(
    name: "INTcoinSDK",
    platforms: [
        .iOS(.v15),
        .macOS(.v12)
    ],
    products: [
        .library(
            name: "INTcoinSDK",
            targets: ["INTcoinSDK"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "INTcoinSDK",
            dependencies: ["INTcoinCore"],
            path: ".",
            sources: ["INTcoinSDK.swift"]
        ),
        .binaryTarget(
            name: "INTcoinCore",
            path: "../../build/libintcoin_core.xcframework"
        )
    ]
)
