#include <gtest/gtest.h>

#include <QString>

#include "viewmodels/FormValidator.hpp"

using devdialogue::viewmodels::FormValidator;

TEST(FormValidatorTest, NicknameNeedsThreeEnglishLettersDigitsOrUnderscores) {
    EXPECT_TRUE(FormValidator::isValidNickname("abc"));
    EXPECT_TRUE(FormValidator::isValidNickname("dev_user01"));

    EXPECT_FALSE(FormValidator::isValidNickname(""));
    EXPECT_FALSE(FormValidator::isValidNickname("ab"));
    EXPECT_FALSE(FormValidator::isValidNickname("Yaroslav (Admin)"));
    EXPECT_FALSE(FormValidator::isValidNickname("two words"));
    EXPECT_FALSE(FormValidator::isValidNickname(QString::fromUtf8("кандидат")));
}

TEST(FormValidatorTest, PasswordNeedsLengthLettersAndDigits) {
    EXPECT_TRUE(FormValidator::isValidPassword("abc12345"));
    EXPECT_TRUE(FormValidator::isValidPassword(QString(71, 'a') + "1"));

    EXPECT_FALSE(FormValidator::isValidPassword("abc1234"));       // 7 characters
    EXPECT_FALSE(FormValidator::isValidPassword("abcdefgh"));      // no digit
    EXPECT_FALSE(FormValidator::isValidPassword("12345678"));      // no letter
    EXPECT_FALSE(FormValidator::isValidPassword(QString(72, 'a') + "1")); // 73 characters
}

TEST(FormValidatorTest, EmailNeedsLocalPartDomainAndTld) {
    EXPECT_TRUE(FormValidator::isValidEmail("you@example.com"));
    EXPECT_TRUE(FormValidator::isValidEmail("first.last+tag@mail.example.org"));

    EXPECT_FALSE(FormValidator::isValidEmail(""));
    EXPECT_FALSE(FormValidator::isValidEmail("no-at-sign"));
    EXPECT_FALSE(FormValidator::isValidEmail("user@localhost"));
    EXPECT_FALSE(FormValidator::isValidEmail("spaced name@example.com"));
    EXPECT_FALSE(FormValidator::isValidEmail("two@@example.com"));
}
