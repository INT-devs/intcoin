// Copyright (c) 2026 The INTcoin Core developers
// Distributed under the MIT software license

#ifndef INTCOIN_QT_ADDRESSBOOK_H
#define INTCOIN_QT_ADDRESSBOOK_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace intcoin {
namespace qt {

/**
 * Contact category
 */
enum class ContactCategory {
    PERSONAL,
    BUSINESS,
    EXCHANGE,
    MINING_POOL,
    MERCHANT,
    OTHER
};

/**
 * Address book contact
 */
struct Contact {
    std::string id;                  // Unique ID
    std::string name;
    std::string address;
    std::string email;
    std::string phone;
    std::string notes;
    ContactCategory category{ContactCategory::PERSONAL};
    std::vector<uint8_t> avatar;     // Optional avatar image
    uint64_t created_at{0};          // Creation timestamp
    uint64_t last_used{0};           // Last transaction timestamp
    uint32_t transaction_count{0};   // Number of transactions
    uint64_t total_sent{0};          // Total amount sent (satoshis)
    uint64_t total_received{0};      // Total amount received (satoshis)
    std::vector<std::string> tags;   // Custom tags
};

/**
 * Contact search filters
 */
struct ContactFilter {
    std::string name_query;
    std::string address_query;
    std::vector<ContactCategory> categories;
    std::vector<std::string> tags;
    bool has_email{false};
    uint32_t min_transaction_count{0};
    uint64_t min_total_amount{0};
};

/**
 * Address Book Manager
 *
 * Manages contacts and addresses for the desktop wallet.
 * Supports categorization, search, import/export, and
 * transaction history tracking per contact.
 */
class AddressBook {
public:
    AddressBook();
    ~AddressBook();

    /**
     * Add new contact
     *
     * @param contact Contact to add
     * @return Contact ID, empty on error
     */
    std::string AddContact(const Contact& contact);

    /**
     * Update existing contact
     *
     * @param contact_id Contact ID
     * @param contact Updated contact data
     * @return True if updated successfully
     */
    bool UpdateContact(const std::string& contact_id, const Contact& contact);

    /**
     * Delete contact
     *
     * @param contact_id Contact ID
     * @return True if deleted successfully
     */
    bool DeleteContact(const std::string& contact_id);

    /**
     * Get contact by ID
     *
     * @param contact_id Contact ID
     * @return Contact (empty if not found)
     */
    Contact GetContact(const std::string& contact_id) const;

    /**
     * Get contact by address
     *
     * @param address Address to look up
     * @return Contact (empty if not found)
     */
    Contact GetContactByAddress(const std::string& address) const;

    /**
     * Get all contacts
     *
     * @return Vector of all contacts
     */
    std::vector<Contact> GetAllContacts() const;

    /**
     * Search contacts
     *
     * @param filter Search filter
     * @return Vector of matching contacts
     */
    std::vector<Contact> SearchContacts(const ContactFilter& filter) const;

    /**
     * Get contacts by category
     *
     * @param category Contact category
     * @return Vector of contacts in category
     */
    std::vector<Contact> GetContactsByCategory(ContactCategory category) const;

    /**
     * Get contacts by tag
     *
     * @param tag Tag name
     * @return Vector of contacts with tag
     */
    std::vector<Contact> GetContactsByTag(const std::string& tag) const;

    /**
     * Add tag to contact
     *
     * @param contact_id Contact ID
     * @param tag Tag to add
     * @return True if added successfully
     */
    bool AddTag(const std::string& contact_id, const std::string& tag);

    /**
     * Remove tag from contact
     *
     * @param contact_id Contact ID
     * @param tag Tag to remove
     * @return True if removed successfully
     */
    bool RemoveTag(const std::string& contact_id, const std::string& tag);

    /**
     * Get all tags
     *
     * @return Vector of all unique tags
     */
    std::vector<std::string> GetAllTags() const;

    /**
     * Update contact transaction stats
     *
     * Called when transaction involving contact is detected
     *
     * @param address Contact address
     * @param amount Transaction amount
     * @param is_outgoing True if sending to contact
     */
    void UpdateTransactionStats(
        const std::string& address,
        uint64_t amount,
        bool is_outgoing
    );

    /**
     * Import contacts from CSV
     *
     * @param csv_data CSV file content
     * @return Number of contacts imported
     */
    uint32_t ImportFromCSV(const std::string& csv_data);

    /**
     * Export contacts to CSV
     *
     * @return CSV string
     */
    std::string ExportToCSV() const;

    /**
     * Import contacts from vCard
     *
     * @param vcard_data vCard file content
     * @return Number of contacts imported
     */
    uint32_t ImportFromVCard(const std::string& vcard_data);

    /**
     * Export contacts to vCard
     *
     * @return vCard string
     */
    std::string ExportToVCard() const;

    /**
     * Get contact count
     */
    size_t GetContactCount() const;

    /**
     * Get recently used contacts
     *
     * @param count Number of contacts to return
     * @return Vector of recent contacts
     */
    std::vector<Contact> GetRecentContacts(size_t count = 10) const;

    /**
     * Get most active contacts
     *
     * @param count Number of contacts to return
     * @return Vector of most active contacts
     */
    std::vector<Contact> GetMostActiveContacts(size_t count = 10) const;

    /**
     * Backup address book
     *
     * @param backup_path Path to backup file
     * @return True if backed up successfully
     */
    bool Backup(const std::string& backup_path) const;

    /**
     * Restore from backup
     *
     * @param backup_path Path to backup file
     * @return True if restored successfully
     */
    bool Restore(const std::string& backup_path);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

/**
 * Get category name
 */
std::string GetCategoryName(ContactCategory category);

/**
 * Parse category from string
 */
ContactCategory ParseCategory(const std::string& name);

} // namespace qt
} // namespace intcoin

#endif // INTCOIN_QT_ADDRESSBOOK_H
