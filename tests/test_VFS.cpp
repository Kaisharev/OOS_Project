#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "../src/core/MountPoint.hpp"
#include "../src/vfs/InMemoryVFS.hpp"
#include "../src/vfs/RWLock.hpp"

// ===================== RWLock =====================

TEST (RWLockTest, PrazanLockDozvoljavaRead) {
    RWLock lock;
    EXPECT_TRUE (lock.try_lock_read ("A1"));
}

TEST (RWLockTest, ViseCitalacaIstovremeno) {
    RWLock lock;
    EXPECT_TRUE (lock.try_lock_read ("A1"));
    EXPECT_TRUE (lock.try_lock_read ("A2"));
    EXPECT_TRUE (lock.try_lock_read ("A3"));
}

TEST (RWLockTest, PisacBlokiraNovogCitaoca) {
    RWLock lock;
    EXPECT_TRUE (lock.try_lock_write ("A1"));
    EXPECT_FALSE (lock.try_lock_read ("A2"));
}

TEST (RWLockTest, CitalacBlokiraNovogPisca) {
    RWLock lock;
    EXPECT_TRUE (lock.try_lock_read ("A1"));
    EXPECT_FALSE (lock.try_lock_write ("A2"));
}

TEST (RWLockTest, PisacBlokiraDrugogPisca) {
    RWLock lock;
    EXPECT_TRUE (lock.try_lock_write ("A1"));
    EXPECT_FALSE (lock.try_lock_write ("A2"));
}

TEST (RWLockTest, UnlockCitaocaOslobadjaZaPisca) {
    RWLock lock;
    lock.try_lock_read ("A1");
    lock.unlock_read ("A1");
    EXPECT_TRUE (lock.try_lock_write ("A2"));
}

TEST (RWLockTest, UnlockPiscaOslobadjaZaCitaoca) {
    RWLock lock;
    lock.try_lock_write ("A1");
    lock.unlock_write ("A1");
    EXPECT_TRUE (lock.try_lock_read ("A2"));
}

TEST (RWLockTest, UnlockJednogCitaocaNeUticeNaDrugog) {
    RWLock lock;
    lock.try_lock_read ("A1");
    lock.try_lock_read ("A2");
    lock.unlock_read ("A1");

    // A2 dalje drži read lock, pa pisac ne smije ući
    EXPECT_FALSE (lock.try_lock_write ("A3"));
}

TEST (RWLockTest, GetHolderVracaPiscaKadaSePise) {
    RWLock lock;
    lock.try_lock_write ("A1");
    EXPECT_EQ (lock.get_holder (), "A1");
}

TEST (RWLockTest, GetHolderVracaPraznoKadaJeSlobodan) {
    RWLock lock;
    EXPECT_EQ (lock.get_holder (), "");
}

// ===================== InMemoryVFS =====================

class InMemoryVfsTest : public ::testing::Test {
    protected:
        std::filesystem::path test_dir;

        void SetUp () override {
            test_dir = std::filesystem::temp_directory_path () / "vfs_test";
            std::filesystem::remove_all (test_dir);

            std::filesystem::create_directories (test_dir / "shared");
            std::filesystem::create_directories (test_dir / "work");

            std::ofstream (test_dir / "shared" / "info.txt") << "shared content";
            std::ofstream (test_dir / "work" / "result.txt") << "";
        }

        void TearDown () override {
            std::filesystem::remove_all (test_dir);
        }
};
TEST_F (InMemoryVfsTest, ReadContentVracaSadrzaj) {
    InMemoryVFS vfs;
    std::string expected = "test content";
    std::ofstream (test_dir / "sample.txt") << expected;

    std::string result = vfs.read_content (test_dir / "sample.txt");
    EXPECT_EQ (result, expected);
}
TEST_F (InMemoryVfsTest, MountNepostojecegFolderaVracaNotFound) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "ne_postoji").string (), "/shared", MountPoint::Mode::RO};
    EXPECT_EQ (vfs.mount (mp), VFSResult::NOT_FOUND);
}

TEST_F (InMemoryVfsTest, MountPostojecegFolderaVracaOk) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    EXPECT_EQ (vfs.mount (mp), VFSResult::OK);
}
/*

TEST_F (InMemoryVfsTest, OpenIReadFajlaIzRoMounta) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/shared/info.txt", "read", "f"), VFSResult::OK);

    std::string content;

    EXPECT_EQ (vfs.read ("A1", "f", content), VFSResult::OK);
    //   std::cout << "Content read from VFS: " << content << std::endl;
    EXPECT_EQ (content, "shared content");
}

TEST_F (InMemoryVfsTest, OpenZaWriteURoMountuVracaPermissionDenied) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/shared/info.txt", "w", "f"), VFSResult::PERMISSION_DENIED);
}

TEST_F (InMemoryVfsTest, OpenNepostojeceputanjeVracaNotFound) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/shared/ne_postoji.txt", "r", "f"), VFSResult::NOT_FOUND);
}

TEST_F (InMemoryVfsTest, WritePaReadURwMountuVracaUpisaniSadrzaj) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "work").string (), "/work", MountPoint::Mode::RW};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/work/result.txt", "w", "f"), VFSResult::OK);
    EXPECT_EQ (vfs.write ("A1", "f", "hello"), VFSResult::OK);
    EXPECT_EQ (vfs.close ("A1", "f"), VFSResult::OK);

    EXPECT_EQ (vfs.open ("A1", "/work/result.txt", "r", "f"), VFSResult::OK);
    std::string content;
    EXPECT_EQ (vfs.read ("A1", "f", content), VFSResult::OK);
    EXPECT_EQ (content, "hello");
}

TEST_F (InMemoryVfsTest, AppendDodajeNaKrajPostojecegSadrzaja) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "work").string (), "/work", MountPoint::Mode::RW};
    vfs.mount (mp);

    vfs.open ("A1", "/work/result.txt", "w", "f1");
    vfs.write ("A1", "f1", "agent A\n");
    vfs.close ("A1", "f1");

    vfs.open ("A2", "/work/result.txt", "a", "f2");
    vfs.append ("A2", "f2", "agent B\n");
    vfs.close ("A2", "f2");

    vfs.open ("A1", "/work/result.txt", "r", "f3");
    std::string content;
    vfs.read ("A1", "f3", content);
    EXPECT_EQ (content, "agent A\nagent B\n");
}

TEST_F (InMemoryVfsTest, DrugiPisacNeMozeOtvoritiDokPrviDrziLock) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "work").string (), "/work", MountPoint::Mode::RW};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/work/result.txt", "w", "f"), VFSResult::OK);
    EXPECT_EQ (vfs.open ("A2", "/work/result.txt", "w", "f"), VFSResult::WOULD_BLOCK);
}

TEST_F (InMemoryVfsTest, ViseCitalacaMozeOtvoritiIstiFajl) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    EXPECT_EQ (vfs.open ("A1", "/shared/info.txt", "r", "f"), VFSResult::OK);
    EXPECT_EQ (vfs.open ("A2", "/shared/info.txt", "r", "f"), VFSResult::OK);
}

TEST_F (InMemoryVfsTest, CloseOslobadjaLockZaSledeceg) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "work").string (), "/work", MountPoint::Mode::RW};
    vfs.mount (mp);

    vfs.open ("A1", "/work/result.txt", "w", "f");
    vfs.close ("A1", "f");

    EXPECT_EQ (vfs.open ("A2", "/work/result.txt", "w", "f"), VFSResult::OK);
}

TEST_F (InMemoryVfsTest, ReadNaNeotvorenomHandleVracaInvalidHandle) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    std::string content;
    EXPECT_EQ (vfs.read ("A1", "nepostojeci_handle", content), VFSResult::INVALID_HANDLE);
}

TEST_F (InMemoryVfsTest, WriteNaHandleOtvorenomZaReadVracaInvalidMode) {
    InMemoryVFS vfs;
    MountPoint mp{(test_dir / "shared").string (), "/shared", MountPoint::Mode::RO};
    vfs.mount (mp);

    vfs.open ("A1", "/shared/info.txt", "r", "f");
    EXPECT_EQ (vfs.write ("A1", "f", "novi sadržaj"), VFSResult::INVALID_MODE);
}
*/