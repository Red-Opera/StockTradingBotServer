package com.springboot.data;

import com.springboot.model.HoldingSnapshot;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;

@Repository
public interface HoldingSnapshotRepository extends JpaRepository<HoldingSnapshot, Long>
{
    // 특정 시간 이후의 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTimeAfterOrderBySnapshotTimeAsc(LocalDateTime startTime);
    
    // 특정 시간 범위의 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTimeBetweenOrderBySnapshotTimeAsc(LocalDateTime startTime, LocalDateTime endTime);
    
    // 특정 종목의 스냅샷 조회
    List<HoldingSnapshot> findByCodeOrderBySnapshotTimeAsc(String code);
    
    // 특정 종목의 특정 시간 이후 스냅샷 조회
    List<HoldingSnapshot> findByCodeAndSnapshotTimeAfterOrderBySnapshotTimeAsc(String code, LocalDateTime startTime);
    
    // 특정 시간의 모든 종목 스냅샷 조회
    List<HoldingSnapshot> findBySnapshotTime(LocalDateTime snapshotTime);
    
    // 가장 최근 스냅샷 시간 조회
    @Query("SELECT MAX(h.snapshotTime) FROM HoldingSnapshot h")
    LocalDateTime findLatestSnapshotTime();
    
    // 최근 스냅샷 조회 (모든 종목)
    @Query("SELECT h FROM HoldingSnapshot h WHERE h.snapshotTime = (SELECT MAX(h2.snapshotTime) FROM HoldingSnapshot h2)")
    List<HoldingSnapshot> findLatestSnapshots();
}
